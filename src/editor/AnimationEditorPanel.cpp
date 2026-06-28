#include "editor/AnimationEditorPanel.h"

#include "editor/EditorCollectionViews.h"
#include "misc/TextureAsset.h"
#include "system/ProjectManager.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace
{
    constexpr int MinFrameTiles = 1;
    constexpr int MaxFrameTiles = 16;
    constexpr float FrameSlotSize = 44.0f;

    std::string makeFrameLabel(int index, const AnimationFrame &frame)
    {
        return "Frame " + std::to_string(index) + " | " + std::to_string(frame.widthInTiles) + "x" + std::to_string(frame.heightInTiles) + " | tiles: " + std::to_string(frame.tiles.size()) + " | " + std::to_string(frame.durationMs) + "ms";
    }

    bool hasAnimationExtension(const std::filesystem::path &path)
    {
        return path.extension().string() == ".ianim";
    }

    bool canDrawTilePreview(const EditorTileset *tileset, const TileInfo *tile)
    {
        return tileset != nullptr && tile != nullptr && tile->tileId >= 0 && tile->tileId < tileset->tileCount && tileset->textureAsset != nullptr && tileset->textureAsset->getImGuiTextureId() != ImTextureID{} && tileset->tileWidth > 0 && tileset->tileHeight > 0 && tileset->columns > 0 && tileset->imageWidth > 0 && tileset->imageHeight > 0;
    }

    ImVec2 getTileUv0(const EditorTileset &tileset, int tileId)
    {
        const int column = tileId % tileset.columns;
        const int row = tileId / tileset.columns;
        return ImVec2(
            static_cast<float>(column * tileset.tileWidth) / static_cast<float>(tileset.imageWidth),
            static_cast<float>(row * tileset.tileHeight) / static_cast<float>(tileset.imageHeight));
    }

    ImVec2 getTileUv1(const EditorTileset &tileset, int tileId)
    {
        const int column = tileId % tileset.columns;
        const int row = tileId / tileset.columns;
        return ImVec2(
            static_cast<float>((column + 1) * tileset.tileWidth) / static_cast<float>(tileset.imageWidth),
            static_cast<float>((row + 1) * tileset.tileHeight) / static_cast<float>(tileset.imageHeight));
    }
}

void AnimationEditorPanel::draw(std::string &statusMessage)
{
    drawAnimationSelector(statusMessage);
    ImGui::Separator();

    drawAnimationFields();

    ImGui::SeparatorText("Tileset");
    drawTilePicker(statusMessage);

    ImGui::SeparatorText("Current Frame");
    drawCurrentFrameEditor(statusMessage);

    ImGui::SeparatorText("Frames");
    drawFrameList(statusMessage);

    ImGui::SeparatorText("Preview");

    if (ImGui::Button("Draw Preview"))
    {
        previewCurrentFrame = 0;
        previewElapsedSeconds = 0.0f;
        previewEnabled = true;
    }

    drawAnimationPreview(statusMessage);

    ImGui::Separator();
    if (ImGui::Button("Save Animation"))
    {
        saveAnimation(statusMessage);
    }
}

void AnimationEditorPanel::drawAnimationFields()
{
    ImGui::InputText("Name", &animationName);
    ImGui::InputText("Output File", &outputFile);

    ImGui::InputInt("Default Duration Ms", &defaultFrameDurationMs);
    defaultFrameDurationMs = std::max(1, defaultFrameDurationMs);
}

void AnimationEditorPanel::drawAnimationSelector(std::string &statusMessage)
{
    if (!animationsScanned)
    {
        refreshAnimationList();
    }

    const char *previewText = "No animation selected";
    if (selectedAnimationIndex >= 0 && selectedAnimationIndex < static_cast<int>(availableAnimations.size()))
    {
        previewText = availableAnimations[static_cast<std::size_t>(selectedAnimationIndex)].c_str();
    }

    if (ImGui::BeginCombo("Animations", previewText))
    {
        if (availableAnimations.empty())
        {
            ImGui::TextDisabled("No .ianim files found.");
        }

        for (std::size_t index = 0; index < availableAnimations.size(); ++index)
        {
            const bool selected = selectedAnimationIndex == static_cast<int>(index);
            if (ImGui::Selectable(availableAnimations[index].c_str(), selected))
            {
                selectedAnimationIndex = static_cast<int>(index);
                loadAnimation(availableAnimationPaths[index], statusMessage);
            }

            if (selected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh Animations"))
    {
        refreshAnimationList();
        statusMessage = "Refreshed animation list.";
    }
}

void AnimationEditorPanel::drawTilePicker(std::string &statusMessage)
{
    if (!spritePickerWidget.hasScannedTilesets())
    {
        spritePickerWidget.refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets"))
    {
        spritePickerWidget.refreshTilesets();
    }

    spritePickerWidget.drawTilesetSelector();

    if (spritePickerWidget.drawSelectedTilesetDetails(statusMessage))
    {
        spritePickerWidget.drawTileGrid("##AnimationEditorTileGrid", false, -1, 220.0f);
    }
}

void AnimationEditorPanel::drawCurrentFrameEditor(std::string &statusMessage)
{
    currentFrame.durationMs = std::max(1, currentFrame.durationMs);
    clampFrame(currentFrame);

    ImGui::InputInt("Frame Duration Ms", &currentFrame.durationMs);
    currentFrame.durationMs = std::max(1, currentFrame.durationMs);

    bool sizeChanged = false;
    sizeChanged |= ImGui::InputInt("Frame Width In Tiles", &currentFrame.widthInTiles);
    sizeChanged |= ImGui::InputInt("Frame Height In Tiles", &currentFrame.heightInTiles);
    if (sizeChanged)
    {
        clampFrame(currentFrame);
        clearTilesOutsideFrame(currentFrame);
        selectedSlotX = std::clamp(selectedSlotX, 0, currentFrame.widthInTiles - 1);
        selectedSlotY = std::clamp(selectedSlotY, 0, currentFrame.heightInTiles - 1);
    }

    ImGui::Text("Selected slot: %d, %d", selectedSlotX, selectedSlotY);
    ImGui::Text("Selected tile: %d", spritePickerWidget.getSelectedTileId());

    const int selectedTileId = spritePickerWidget.getSelectedTileId();
    if (selectedTileId < 0)
    {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Set Selected Tile"))
    {
        setTileAt(currentFrame, selectedSlotX, selectedSlotY, selectedTileId);
        statusMessage = "Set frame slot to tile " + std::to_string(selectedTileId) + ".";
    }

    if (selectedTileId < 0)
    {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Slot"))
    {
        clearTileAt(currentFrame, selectedSlotX, selectedSlotY);
        statusMessage = "Cleared frame slot.";
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear Frame"))
    {
        currentFrame.tiles.clear();
        statusMessage = "Cleared current frame.";
    }

    drawFrameSlots("##CurrentAnimationFrameSlots", currentFrame, true, statusMessage);

    if (ImGui::Button("Add Current Frame"))
    {
        addCurrentFrame(statusMessage);
    }
}

void AnimationEditorPanel::drawFrameList(std::string &statusMessage)
{
    std::vector<ListViewItem> items;
    items.reserve(animationFrames.size());

    for (std::size_t index = 0; index < animationFrames.size(); ++index)
    {
        ListViewItem item;
        item.id = "animation_frame_" + std::to_string(index);
        item.label = makeFrameLabel(static_cast<int>(index), animationFrames[index]);
        items.push_back(std::move(item));
    }

    if (selectedFrameIndex >= static_cast<int>(animationFrames.size()))
    {
        selectedFrameIndex = static_cast<int>(animationFrames.size()) - 1;
    }

    ListViewOptions options;
    options.height = 150.0f;
    options.emptyText = "No animation frames.";

    EditorCollectionViews::drawListView(
        "##AnimationFramesList",
        items,
        selectedFrameIndex,
        options);

    if (selectedFrameIndex < 0 || selectedFrameIndex >= static_cast<int>(animationFrames.size()))
    {
        return;
    }

    AnimationFrame &selectedFrame = animationFrames[static_cast<std::size_t>(selectedFrameIndex)];
    clampFrame(selectedFrame);
    clearTilesOutsideFrame(selectedFrame);

    ImGui::InputInt("Selected Duration Ms", &selectedFrame.durationMs);
    selectedFrame.durationMs = std::max(1, selectedFrame.durationMs);

    ImGui::Text(
        "Size: %dx%d, tiles: %zu",
        selectedFrame.widthInTiles,
        selectedFrame.heightInTiles,
        selectedFrame.tiles.size());

    drawFrameSlots("##SelectedAnimationFrameSlots", selectedFrame, false, statusMessage);

    if (ImGui::Button("Load Selected Into Current"))
    {
        currentFrame = selectedFrame;
        selectedSlotX = 0;
        selectedSlotY = 0;
        statusMessage = "Loaded selected frame into current frame.";
    }

    ImGui::SameLine();
    if (ImGui::Button("Remove Selected Frame"))
    {
        animationFrames.erase(animationFrames.begin() + selectedFrameIndex);

        if (selectedFrameIndex >= static_cast<int>(animationFrames.size()))
        {
            selectedFrameIndex = static_cast<int>(animationFrames.size()) - 1;
        }

        statusMessage = "Removed animation frame.";
    }
}

void AnimationEditorPanel::drawFrameSlots(
    const char *id,
    AnimationFrame &frame,
    bool editable,
    std::string &statusMessage)
{
    clampFrame(frame);

    const float rowSpacing = ImGui::GetStyle().ItemSpacing.y;
    const float childPadding = ImGui::GetStyle().WindowPadding.y * 2.0f;
    const float childHeight =
        (static_cast<float>(frame.heightInTiles) * FrameSlotSize) + (static_cast<float>(std::max(0, frame.heightInTiles - 1)) * rowSpacing) + childPadding;

    if (ImGui::BeginChild(
            id,
            ImVec2(0.0f, childHeight),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar))
    {
        const EditorTileset *tileset = spritePickerWidget.getSelectedTileset();
        ImDrawList *drawList = ImGui::GetWindowDrawList();

        for (int y = 0; y < frame.heightInTiles; ++y)
        {
            for (int x = 0; x < frame.widthInTiles; ++x)
            {
                ImGui::PushID((y * frame.widthInTiles) + x);

                const TileInfo *tile = findTileAt(frame, x, y);
                const bool selected = editable && selectedSlotX == x && selectedSlotY == y;

                if (ImGui::InvisibleButton("##FrameSlot", ImVec2(FrameSlotSize, FrameSlotSize)))
                {
                    if (editable)
                    {
                        selectedSlotX = x;
                        selectedSlotY = y;
                        statusMessage = "Selected frame slot " + std::to_string(x) + ", " + std::to_string(y) + ".";
                    }
                }

                const ImVec2 slotMin = ImGui::GetItemRectMin();
                const ImVec2 slotMax = ImGui::GetItemRectMax();
                const bool hovered = ImGui::IsItemHovered();
                const ImU32 fillColor = selected
                                            ? ImGui::GetColorU32(ImGuiCol_HeaderActive)
                                        : hovered
                                            ? ImGui::GetColorU32(ImGuiCol_HeaderHovered)
                                            : ImGui::GetColorU32(ImGuiCol_Button);

                drawList->AddRectFilled(slotMin, slotMax, fillColor, 3.0f);

                if (canDrawTilePreview(tileset, tile))
                {
                    constexpr float imagePadding = 3.0f;
                    const ImVec2 imageMin(slotMin.x + imagePadding, slotMin.y + imagePadding);
                    const ImVec2 imageMax(slotMax.x - imagePadding, slotMax.y - imagePadding);
                    drawList->AddImage(
                        tileset->textureAsset->getImGuiTextureId(),
                        imageMin,
                        imageMax,
                        getTileUv0(*tileset, tile->tileId),
                        getTileUv1(*tileset, tile->tileId));
                }
                else
                {
                    const char *emptyLabel = tile == nullptr ? "-" : "?";
                    const ImVec2 textSize = ImGui::CalcTextSize(emptyLabel);
                    drawList->AddText(
                        ImVec2(
                            slotMin.x + ((FrameSlotSize - textSize.x) * 0.5f),
                            slotMin.y + ((FrameSlotSize - textSize.y) * 0.5f)),
                        ImGui::GetColorU32(ImGuiCol_TextDisabled),
                        emptyLabel);
                }

                drawList->AddRect(
                    slotMin,
                    slotMax,
                    ImGui::GetColorU32(selected ? ImGuiCol_TextSelectedBg : ImGuiCol_Border),
                    3.0f,
                    0,
                    selected ? 2.0f : 1.0f);

                if (x + 1 < frame.widthInTiles)
                {
                    ImGui::SameLine();
                }

                ImGui::PopID();
            }
        }
    }

    ImGui::EndChild();
}

bool AnimationEditorPanel::saveAnimation(std::string &statusMessage)
{
    if (animationName.empty())
    {
        statusMessage = "Animation name is required.";
        return false;
    }

    if (animationFrames.empty())
    {
        statusMessage = "Animation needs at least one frame.";
        return false;
    }

    const EditorTileset *tileset = spritePickerWidget.getSelectedTileset();
    if (tileset == nullptr)
    {
        statusMessage = "Select a tileset before saving animation.";
        return false;
    }

    namespace fs = std::filesystem;

    fs::path outputPath = outputFile;
    outputPath.replace_extension(".ianim");

    if (!outputPath.is_absolute())
    {
        outputPath = ProjectManager::getInstance().getAssetsPath() / "Animations" / outputPath;
    }

    std::error_code error;
    fs::create_directories(outputPath.parent_path(), error);
    if (error)
    {
        statusMessage = "Failed to create animation directory: " + error.message();
        return false;
    }

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement *root = document.NewElement("ianim");
    root->SetAttribute("version", "1.0");
    root->SetAttribute("name", animationName.c_str());
    root->SetAttribute("tileset", fs::path(tileset->path).filename().string().c_str());
    document.InsertEndChild(root);

    for (const AnimationFrame &frameData : animationFrames)
    {
        tinyxml2::XMLElement *frameElement = document.NewElement("frame");
        frameElement->SetAttribute("duration", std::max(1, frameData.durationMs));
        frameElement->SetAttribute("width", std::clamp(frameData.widthInTiles, MinFrameTiles, MaxFrameTiles));
        frameElement->SetAttribute("height", std::clamp(frameData.heightInTiles, MinFrameTiles, MaxFrameTiles));

        for (const TileInfo &tileData : frameData.tiles)
        {
            if (tileData.tileId < 0 || tileData.x < 0 || tileData.y < 0 || tileData.x >= frameData.widthInTiles || tileData.y >= frameData.heightInTiles)
            {
                continue;
            }

            tinyxml2::XMLElement *tileElement = document.NewElement("tile");
            tileElement->SetAttribute("id", tileData.tileId);
            tileElement->SetAttribute("x", tileData.x);
            tileElement->SetAttribute("y", tileData.y);
            frameElement->InsertEndChild(tileElement);
        }

        root->InsertEndChild(frameElement);
    }

    const tinyxml2::XMLError result = document.SaveFile(outputPath.string().c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
        statusMessage = "Failed to save animation: " + std::string(document.ErrorStr());
        return false;
    }

    refreshAnimationList();
    for (std::size_t index = 0; index < availableAnimationPaths.size(); ++index)
    {
        if (availableAnimationPaths[index].filename() == outputPath.filename())
        {
            selectedAnimationIndex = static_cast<int>(index);
            break;
        }
    }

    statusMessage = "Saved animation: " + outputPath.filename().string();
    return true;
}

void AnimationEditorPanel::addCurrentFrame(std::string &statusMessage)
{
    clampFrame(currentFrame);
    clearTilesOutsideFrame(currentFrame);

    if (currentFrame.tiles.empty())
    {
        statusMessage = "Current frame needs at least one tile.";
        return;
    }

    currentFrame.durationMs = std::max(1, currentFrame.durationMs);
    animationFrames.push_back(currentFrame);
    selectedFrameIndex = static_cast<int>(animationFrames.size()) - 1;
    currentFrame.durationMs = defaultFrameDurationMs;
    statusMessage = "Added animation frame.";
}

void AnimationEditorPanel::clampFrame(AnimationFrame &frame)
{
    frame.widthInTiles = std::clamp(frame.widthInTiles, MinFrameTiles, MaxFrameTiles);
    frame.heightInTiles = std::clamp(frame.heightInTiles, MinFrameTiles, MaxFrameTiles);
    frame.durationMs = std::max(1, frame.durationMs);
}

void AnimationEditorPanel::clearTilesOutsideFrame(AnimationFrame &frame)
{
    frame.tiles.erase(
        std::remove_if(
            frame.tiles.begin(),
            frame.tiles.end(),
            [&frame](const TileInfo &tile)
            {
                return tile.x < 0 || tile.y < 0 || tile.x >= frame.widthInTiles || tile.y >= frame.heightInTiles;
            }),
        frame.tiles.end());
}

TileInfo *AnimationEditorPanel::findTileAt(AnimationFrame &frame, int x, int y)
{
    const auto iterator = std::find_if(
        frame.tiles.begin(),
        frame.tiles.end(),
        [x, y](const TileInfo &tile)
        {
            return tile.x == x && tile.y == y;
        });

    return iterator == frame.tiles.end() ? nullptr : &(*iterator);
}

const TileInfo *AnimationEditorPanel::findTileAt(const AnimationFrame &frame, int x, int y) const
{
    const auto iterator = std::find_if(
        frame.tiles.begin(),
        frame.tiles.end(),
        [x, y](const TileInfo &tile)
        {
            return tile.x == x && tile.y == y;
        });

    return iterator == frame.tiles.end() ? nullptr : &(*iterator);
}

void AnimationEditorPanel::setTileAt(AnimationFrame &frame, int x, int y, int tileId)
{
    if (tileId < 0 || x < 0 || y < 0 || x >= frame.widthInTiles || y >= frame.heightInTiles)
    {
        return;
    }

    TileInfo *existingTile = findTileAt(frame, x, y);
    if (existingTile != nullptr)
    {
        existingTile->tileId = tileId;
        return;
    }

    frame.tiles.push_back(TileInfo{tileId, x, y});
}

void AnimationEditorPanel::clearTileAt(AnimationFrame &frame, int x, int y)
{
    frame.tiles.erase(
        std::remove_if(
            frame.tiles.begin(),
            frame.tiles.end(),
            [x, y](const TileInfo &tile)
            {
                return tile.x == x && tile.y == y;
            }),
        frame.tiles.end());
}

bool AnimationEditorPanel::loadAnimation(const std::filesystem::path &filePath, std::string &statusMessage)
{
    tinyxml2::XMLDocument document;
    const tinyxml2::XMLError result = document.LoadFile(filePath.string().c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
        statusMessage = "Failed to load animation: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement *root = document.FirstChildElement("ianim");
    if (root == nullptr)
    {
        statusMessage = "Animation file is missing ianim root: " + filePath.filename().string();
        return false;
    }

    const char *loadedName = root->Attribute("name");
    animationName = loadedName == nullptr || loadedName[0] == '\0'
                        ? filePath.stem().string()
                        : loadedName;
    outputFile = filePath.filename().string();

    animationFrames.clear();
    for (const tinyxml2::XMLElement *frameElement = root->FirstChildElement("frame");
         frameElement != nullptr;
         frameElement = frameElement->NextSiblingElement("frame"))
    {
        AnimationFrame frame;
        frame.durationMs = std::max(1, frameElement->IntAttribute("duration", defaultFrameDurationMs));
        frame.widthInTiles = frameElement->IntAttribute("width", 1);
        frame.heightInTiles = frameElement->IntAttribute("height", 1);
        clampFrame(frame);

        for (const tinyxml2::XMLElement *tileElement = frameElement->FirstChildElement("tile");
             tileElement != nullptr;
             tileElement = tileElement->NextSiblingElement("tile"))
        {
            TileInfo tile;
            tile.tileId = tileElement->IntAttribute("id", -1);
            tile.x = tileElement->IntAttribute("x", 0);
            tile.y = tileElement->IntAttribute("y", 0);

            if (tile.tileId >= 0 && tile.x >= 0 && tile.y >= 0 && tile.x < frame.widthInTiles && tile.y < frame.heightInTiles)
            {
                frame.tiles.push_back(tile);
            }
        }

        clearTilesOutsideFrame(frame);
        animationFrames.push_back(std::move(frame));
    }

    selectedFrameIndex = animationFrames.empty() ? -1 : 0;
    currentFrame = selectedFrameIndex >= 0
                       ? animationFrames[static_cast<std::size_t>(selectedFrameIndex)]
                       : AnimationFrame{};
    selectedSlotX = 0;
    selectedSlotY = 0;

    const char *tilesetFilename = root->Attribute("tileset");
    if (tilesetFilename != nullptr && tilesetFilename[0] != '\0')
    {
        if (!spritePickerWidget.setSelectedTilesetByFilename(tilesetFilename))
        {
            statusMessage = "Loaded animation, but tileset was not found: " + std::string(tilesetFilename);
            return true;
        }
    }

    statusMessage = "Loaded animation: " + filePath.filename().string();
    return true;
}

void AnimationEditorPanel::refreshAnimationList()
{
    availableAnimations.clear();
    availableAnimationPaths.clear();

    const std::filesystem::path animationsRoot =
        ProjectManager::getInstance().getAssetsPath() / "Animations";

    std::error_code error;
    if (!std::filesystem::exists(animationsRoot, error) || !std::filesystem::is_directory(animationsRoot, error))
    {
        selectedAnimationIndex = -1;
        animationsScanned = true;
        return;
    }

    std::vector<std::filesystem::path> animationPaths;
    for (const std::filesystem::directory_entry &entry :
         std::filesystem::recursive_directory_iterator(animationsRoot, error))
    {
        if (error)
        {
            break;
        }

        if (entry.is_regular_file(error) && hasAnimationExtension(entry.path()))
        {
            animationPaths.push_back(entry.path());
        }
    }

    std::sort(animationPaths.begin(), animationPaths.end());

    for (const std::filesystem::path &path : animationPaths)
    {
        availableAnimationPaths.push_back(path);
        availableAnimations.push_back(path.filename().string());
    }

    if (selectedAnimationIndex >= static_cast<int>(availableAnimations.size()))
    {
        selectedAnimationIndex = static_cast<int>(availableAnimations.size()) - 1;
    }

    animationsScanned = true;
}

void AnimationEditorPanel::drawAnimationPreview(std::string &statusMessage)
{
    (void)statusMessage;

    const int numberOfFrames = static_cast<int>(animationFrames.size());
    int tileMaxWidth = 1;
    int tileMaxHeight = 1;

    for (const AnimationFrame &animationFrame : animationFrames)
    {
        tileMaxWidth = std::max(tileMaxWidth, animationFrame.widthInTiles);
        tileMaxHeight = std::max(tileMaxHeight, animationFrame.heightInTiles);
    }

    const EditorTileset *tileset = spritePickerWidget.getSelectedTileset();
    const bool hasUsableTileset =
        tileset != nullptr
        && tileset->textureAsset != nullptr
        && tileset->textureAsset->getImGuiTextureId() != ImTextureID{}
        && tileset->tileWidth > 0
        && tileset->tileHeight > 0;

    const float previewScale = 2.0f;
    const float tileWidth = hasUsableTileset
        ? static_cast<float>(tileset->tileWidth) * previewScale
        : 32.0f;
    const float tileHeight = hasUsableTileset
        ? static_cast<float>(tileset->tileHeight) * previewScale
        : 32.0f;

    const ImVec2 previewSize(
        static_cast<float>(tileMaxWidth) * tileWidth,
        static_cast<float>(tileMaxHeight) * tileHeight);
    const ImVec2 previewChildSize(
        0.0f,
        previewSize.y + (ImGui::GetStyle().WindowPadding.y * 2.0f));

    if (ImGui::BeginChild(
            "AnimationPreview",
            previewChildSize,
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_NoScrollbar))
    {
        ImDrawList *drawList = ImGui::GetWindowDrawList();
        const ImVec2 cursorPos = ImGui::GetCursorScreenPos();
        const ImVec2 availableSize = ImGui::GetContentRegionAvail();
        const float canvasOffsetX = std::max(0.0f, (availableSize.x - previewSize.x) * 0.5f);
        const ImVec2 canvasPos(cursorPos.x + canvasOffsetX, cursorPos.y);
        const ImVec2 canvasMax(canvasPos.x + previewSize.x, canvasPos.y + previewSize.y);

        drawList->AddRect(
            canvasPos,
            canvasMax,
            ImGui::GetColorU32(ImGuiCol_Border),
            3.0f,
            0,
            1.5f);

        auto drawCenteredPreviewText = [&](const char *text)
        {
            const ImVec2 textSize = ImGui::CalcTextSize(text);
            drawList->AddText(
                ImVec2(
                    canvasPos.x + ((previewSize.x - textSize.x) * 0.5f),
                    canvasPos.y + ((previewSize.y - textSize.y) * 0.5f)),
                ImGui::GetColorU32(ImGuiCol_TextDisabled),
                text);
        };

        if (numberOfFrames <= 0)
        {
            previewEnabled = false;
            drawCenteredPreviewText("No animation frames.");
        }
        else if (!hasUsableTileset)
        {
            drawCenteredPreviewText("No tileset selected.");
        }
        else if (!previewEnabled)
        {
            drawCenteredPreviewText("Preview");
        }
        else
        {
            previewCurrentFrame = std::clamp(previewCurrentFrame, 0, numberOfFrames - 1);
            AnimationFrame &currentFrameData = animationFrames[previewCurrentFrame];

            previewElapsedSeconds += ImGui::GetIO().DeltaTime;
            const float frameDurationSeconds =
                static_cast<float>(std::max(1, currentFrameData.durationMs)) / 1000.0f;

            if (previewElapsedSeconds >= frameDurationSeconds)
            {
                previewElapsedSeconds = 0.0f;
                previewCurrentFrame++;
            }

            if (previewCurrentFrame >= numberOfFrames)
            {
                previewCurrentFrame = 0;
                previewElapsedSeconds = 0.0f;
                previewEnabled = false;
            }

            const float frameWidth = static_cast<float>(currentFrameData.widthInTiles) * tileWidth;
            const float frameHeight = static_cast<float>(currentFrameData.heightInTiles) * tileHeight;
            const float offsetX = (previewSize.x - frameWidth) * 0.5f;
            const float offsetY = (previewSize.y - frameHeight) * 0.5f;

            for (const TileInfo &tile : currentFrameData.tiles)
            {
                if (!canDrawTilePreview(tileset, &tile))
                {
                    continue;
                }

                const ImVec2 p0(
                    canvasPos.x + offsetX + static_cast<float>(tile.x) * tileWidth,
                    canvasPos.y + offsetY + static_cast<float>(tile.y) * tileHeight);

                const ImVec2 p1(
                    p0.x + tileWidth,
                    p0.y + tileHeight);

                drawList->AddImage(
                    tileset->textureAsset->getImGuiTextureId(),
                    p0,
                    p1,
                    getTileUv0(*tileset, tile.tileId),
                    getTileUv1(*tileset, tile.tileId));
            }
        }

        // Reserve space so ImGui knows the child content size
        ImGui::Dummy(previewSize);
    }

    ImGui::EndChild();
}
