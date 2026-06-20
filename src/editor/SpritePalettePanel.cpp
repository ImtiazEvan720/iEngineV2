#include "editor/SpritePalettePanel.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "tinyxml2.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <iostream>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

std::string getAttribute(const tinyxml2::XMLElement* element, const char* name, const std::string& fallback = "") {
    if (element == nullptr) {
        return fallback;
    }

    const char* value = element->Attribute(name);
    return value == nullptr ? fallback : value;
}

std::filesystem::path resolveRelativePath(const std::filesystem::path& baseFile, const std::string& relativePath) {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return (baseFile.parent_path() / path).lexically_normal();
}

std::filesystem::path findAssetsRoot() {
    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        std::filesystem::path path(asset->getPath());
        std::filesystem::path parent = path.parent_path();

        while (!parent.empty()) {
            if (parent.filename().string() == "Assets") {
                return parent;
            }

            const std::filesystem::path nextParent = parent.parent_path();
            if (nextParent == parent) {
                break;
            }

            parent = nextParent;
        }
    }

    return ProjectManager::getInstance().getAssetsPath();
}

tinyxml2::XMLElement* findTileElementById(tinyxml2::XMLElement* tilesetRoot, int tileId) {
    for (tinyxml2::XMLElement* tile = tilesetRoot == nullptr ? nullptr : tilesetRoot->FirstChildElement("tile");
         tile != nullptr;
         tile = tile->NextSiblingElement("tile")) {
        if (tile->IntAttribute("id", -1) == tileId) {
            return tile;
        }
    }

    return nullptr;
}
}

void SpritePalettePanel::draw(std::string& statusMessage) {
    if (!tilesetsScanned) {
        refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets")) {
        refreshTilesets();
    }

    drawTilesetSelector();
    drawSelectedTilesetGrid(statusMessage);
}

void SpritePalettePanel::refreshTilesets() {
    namespace fs = std::filesystem;

    tilesets.clear();
    selectedTilesetIndex = -1;
    selectedTileId = -1;

    const fs::path assetsRoot = findAssetsRoot();
    if (!fs::exists(assetsRoot) || !fs::is_directory(assetsRoot)) {
        std::cerr << "Assets directory not found for editor tilesets: "
                  << assetsRoot.string() << std::endl;
        tilesetsScanned = true;
        return;
    }

    std::vector<fs::path> tilesetPaths;
    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(assetsRoot)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        if (toLower(entry.path().extension().string()) == ".tsx") {
            tilesetPaths.push_back(entry.path());
        }
    }

    std::sort(tilesetPaths.begin(), tilesetPaths.end());

    for (const fs::path& tilesetPath : tilesetPaths) {
        tinyxml2::XMLDocument document;
        if (document.LoadFile(tilesetPath.string().c_str()) != tinyxml2::XML_SUCCESS) {
            std::cerr << "Failed to load editor tileset: " << tilesetPath.string()
                      << " - " << document.ErrorStr() << std::endl;
            continue;
        }

        const tinyxml2::XMLElement* tilesetRoot = document.FirstChildElement("tileset");
        if (tilesetRoot == nullptr) {
            std::cerr << "Editor tileset is missing root: " << tilesetPath.string() << std::endl;
            continue;
        }

        EditorTileset tileset;
        tileset.path = tilesetPath.string();
        tileset.name = getAttribute(tilesetRoot, "name", tilesetPath.stem().string());
        tileset.tileWidth = tilesetRoot->IntAttribute("tilewidth");
        tileset.tileHeight = tilesetRoot->IntAttribute("tileheight");
        tileset.columns = tilesetRoot->IntAttribute("columns");
        tileset.tileCount = tilesetRoot->IntAttribute("tilecount");

        const tinyxml2::XMLElement* image = tilesetRoot->FirstChildElement("image");
        if (image != nullptr) {
            const std::string imageSource = getAttribute(image, "source");
            tileset.imagePath = resolveRelativePath(tilesetPath, imageSource).string();
            tileset.imageFilename = fs::path(imageSource).filename().string();
            tileset.imageWidth = image->IntAttribute("width");
            tileset.imageHeight = image->IntAttribute("height");
            tileset.textureAsset = AssetManager::getInstance().getTextureAssetByName(tileset.imageFilename);
        }

        for (const tinyxml2::XMLElement* tile = tilesetRoot->FirstChildElement("tile");
             tile != nullptr;
             tile = tile->NextSiblingElement("tile")) {
            const int ownerTileId = tile->IntAttribute("id", -1);
            if (ownerTileId < 0) {
                continue;
            }

            const tinyxml2::XMLElement* animation = tile->FirstChildElement("animation");
            if (animation == nullptr) {
                continue;
            }

            EditorTileAnimation editorAnimation;
            for (const tinyxml2::XMLElement* frame = animation->FirstChildElement("frame");
                 frame != nullptr;
                 frame = frame->NextSiblingElement("frame")) {
                EditorAnimationFrame editorFrame;
                editorFrame.tileId = frame->IntAttribute("tileid", ownerTileId);
                editorFrame.durationMs = std::max(1, frame->IntAttribute("duration", 200));
                editorAnimation.frames.push_back(editorFrame);
            }

            if (!editorAnimation.frames.empty()) {
                tileset.animations[ownerTileId] = std::move(editorAnimation);
            }
        }

        tilesets.push_back(tileset);
    }

    if (!tilesets.empty()) {
        selectedTilesetIndex = 0;
        animationOwnerTileId = -1;
    }

    tilesetsScanned = true;
}

void SpritePalettePanel::drawTilesetSelector() {
    if (tilesets.empty()) {
        ImGui::TextUnformatted("No .tsx files found in Assets.");
        return;
    }

    if (selectedTilesetIndex < 0 || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        selectedTilesetIndex = 0;
    }

    const EditorTileset& selectedTileset = tilesets[static_cast<std::size_t>(selectedTilesetIndex)];
    const char* previewText = selectedTileset.name.empty() ? selectedTileset.path.c_str() : selectedTileset.name.c_str();

    if (ImGui::BeginCombo("Tileset", previewText)) {
        for (std::size_t index = 0; index < tilesets.size(); ++index) {
            const bool isSelected = selectedTilesetIndex == static_cast<int>(index);
            const std::string& label = tilesets[index].name.empty() ? tilesets[index].path : tilesets[index].name;

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedTilesetIndex = static_cast<int>(index);
                selectedTileId = -1;
                animationOwnerTileId = -1;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void SpritePalettePanel::drawSelectedTilesetGrid(std::string& statusMessage) {
    if (tilesets.empty()
        || selectedTilesetIndex < 0
        || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        return;
    }

    EditorTileset& tileset = tilesets[static_cast<std::size_t>(selectedTilesetIndex)];

    ImGui::Text("File: %s", std::filesystem::path(tileset.path).filename().string().c_str());
    ImGui::Text("Image: %s", tileset.imageFilename.empty() ? "none" : tileset.imageFilename.c_str());
    ImGui::Text("Tile size: %dx%d, tiles: %d, columns: %d",
                tileset.tileWidth,
                tileset.tileHeight,
                tileset.tileCount,
                tileset.columns);

    if (tileset.textureAsset == nullptr || tileset.textureAsset->getImGuiTextureId() == ImTextureID{}) {
        ImGui::TextUnformatted("Texture asset is not loaded for this tileset.");
        return;
    }

    if (tileset.tileWidth <= 0
        || tileset.tileHeight <= 0
        || tileset.tileCount <= 0
        || tileset.columns <= 0
        || tileset.imageWidth <= 0
        || tileset.imageHeight <= 0) {
        ImGui::TextUnformatted("Tileset metadata is incomplete.");
        return;
    }

    ImGui::SliderFloat("Preview Scale", &tilePreviewScale, 1.0f, 6.0f, "%.1fx");

    if (selectedTileId >= 0) {
        ImGui::Text("Selected tile id: %d", selectedTileId);
    } else {
        ImGui::TextUnformatted("Selected tile id: none");
    }

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    drawSelectedTileAnimationEditor(tileset, statusMessage);

    const ImTextureID textureId = tileset.textureAsset->getImGuiTextureId();
    const ImVec2 previewSize(
        static_cast<float>(tileset.tileWidth) * tilePreviewScale,
        static_cast<float>(tileset.tileHeight) * tilePreviewScale
    );

    if (ImGui::BeginChild(
            "##TilesetGrid",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        for (int tileId = 0; tileId < tileset.tileCount; ++tileId) {
            const int column = tileId % tileset.columns;
            const int row = tileId / tileset.columns;
            const float sourceX = static_cast<float>(column * tileset.tileWidth);
            const float sourceY = static_cast<float>(row * tileset.tileHeight);

            const ImVec2 uv0(
                sourceX / static_cast<float>(tileset.imageWidth),
                sourceY / static_cast<float>(tileset.imageHeight)
            );
            const ImVec2 uv1(
                (sourceX + static_cast<float>(tileset.tileWidth)) / static_cast<float>(tileset.imageWidth),
                (sourceY + static_cast<float>(tileset.tileHeight)) / static_cast<float>(tileset.imageHeight)
            );

            ImGui::PushID(tileId);

            const bool selected = selectedTileId == tileId;
            if (selected) {
                ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));
            }

            if (ImGui::ImageButton("tile", textureId, previewSize, uv0, uv1)) {
                selectedTileId = tileId;
            }

            const ImVec2 tileMin = ImGui::GetItemRectMin();
            const ImVec2 tileMax = ImGui::GetItemRectMax();
            const auto animationIterator = tileset.animations.find(tileId);
            const bool animatedOwner =
                animationIterator != tileset.animations.end()
                && !animationIterator->second.frames.empty();
            const bool activeOwner = animationOwnerTileId == tileId;

            if (animatedOwner || activeOwner) {
                const char* label = animatedOwner ? "OWNER" : "OWNER*";
                const ImVec2 textSize = ImGui::CalcTextSize(label);
                const float labelHeight = textSize.y + 4.0f;
                const ImU32 backgroundColor = animatedOwner
                    ? IM_COL32(33, 120, 64, 220)
                    : IM_COL32(120, 90, 28, 210);
                ImDrawList* drawList = ImGui::GetWindowDrawList();
                drawList->AddRectFilled(
                    ImVec2(tileMin.x, tileMax.y - labelHeight),
                    tileMax,
                    backgroundColor
                );
                drawList->AddText(
                    ImVec2(tileMin.x + std::max(2.0f, (previewSize.x - textSize.x) * 0.5f), tileMax.y - labelHeight + 2.0f),
                    IM_COL32(255, 255, 255, 245),
                    label
                );
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                const TileDragPayload payload{
                    selectedTilesetIndex,
                    tileId
                };

                ImGui::SetDragDropPayload(EditorTileDrag::PayloadType, &payload, sizeof(payload));
                ImGui::Text("Tile id: %d", tileId);

                const ImVec2 mousePosition = ImGui::GetMousePos();
                Renderer& renderer = Renderer::getInstance();
                const float renderScale = renderer.getRenderScale();
                const float cameraZoom = renderer.getCamera().getZoom();

                const ImVec2 halfSize(
                    tileset.tileWidth * renderScale * cameraZoom * 0.5f,
                    tileset.tileHeight * renderScale * cameraZoom * 0.5f
                );
                ImGui::GetForegroundDrawList()->AddImage(
                    textureId,
                    ImVec2(mousePosition.x - halfSize.x, mousePosition.y - halfSize.y),
                    ImVec2(mousePosition.x + halfSize.x, mousePosition.y + halfSize.y),
                    uv0,
                    uv1
                );

                ImGui::EndDragDropSource();
            }

            if (selected) {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered()) {
                if (animatedOwner) {
                    ImGui::SetTooltip(
                        "tile id: %d\nanimation owner\nframes: %zu",
                        tileId,
                        animationIterator->second.frames.size()
                    );
                } else {
                    ImGui::SetTooltip("tile id: %d", tileId);
                }
            }

            ImGui::PopID();

            if ((tileId + 1) % tileset.columns != 0) {
                ImGui::SameLine();
            }
        }
    }

    ImGui::EndChild();
}

void SpritePalettePanel::drawSelectedTileAnimationEditor(EditorTileset& tileset, std::string& statusMessage) {
    ImGui::Separator();

    if (selectedTileId < 0) {
        ImGui::TextUnformatted("Select a tile to edit animation frames.");
        return;
    }

    if (animationOwnerTileId < 0 || animationOwnerTileId >= tileset.tileCount) {
        animationOwnerTileId = selectedTileId;
    }

    ImGui::Text("Animation owner tile: %d", animationOwnerTileId);
    ImGui::SameLine();
    if (ImGui::Button("Use Selected As Owner")) {
        animationOwnerTileId = selectedTileId;
    }

    ImGui::InputInt("Owner Tile Id", &animationOwnerTileId);
    animationOwnerTileId = std::clamp(animationOwnerTileId, 0, std::max(0, tileset.tileCount - 1));

    ImGui::InputInt("Frame Duration Ms", &newAnimationFrameDurationMs);
    newAnimationFrameDurationMs = std::max(1, newAnimationFrameDurationMs);
    const float framesPerSecond = 1000.0f / static_cast<float>(newAnimationFrameDurationMs);
    ImGui::Text("Speed: %.2f FPS", framesPerSecond);

    if (ImGui::Button("Add Selected Tile As Frame")) {
        EditorTileAnimation& animation = tileset.animations[animationOwnerTileId];
        animation.frames.push_back(EditorAnimationFrame{
            selectedTileId,
            newAnimationFrameDurationMs
        });
        statusMessage =
            "Added tile " + std::to_string(selectedTileId)
            + " as animation frame for tile " + std::to_string(animationOwnerTileId) + ".";
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear Animation")) {
        tileset.animations.erase(animationOwnerTileId);
        statusMessage = "Cleared animation for tile " + std::to_string(animationOwnerTileId) + ".";
    }

    auto animationIterator = tileset.animations.find(animationOwnerTileId);
    if (animationIterator == tileset.animations.end() || animationIterator->second.frames.empty()) {
        ImGui::TextUnformatted("No frames for this tile yet.");
    } else {
        EditorTileAnimation& animation = animationIterator->second;
        int removeFrameIndex = -1;

        for (std::size_t frameIndex = 0; frameIndex < animation.frames.size(); ++frameIndex) {
            EditorAnimationFrame& frame = animation.frames[frameIndex];
            ImGui::PushID(static_cast<int>(frameIndex));

            ImGui::Text("Frame %zu", frameIndex);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(96.0f);
            ImGui::InputInt("Tile", &frame.tileId);
            frame.tileId = std::clamp(frame.tileId, 0, std::max(0, tileset.tileCount - 1));

            ImGui::SameLine();
            ImGui::SetNextItemWidth(112.0f);
            ImGui::InputInt("Duration", &frame.durationMs);
            frame.durationMs = std::max(1, frame.durationMs);

            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                removeFrameIndex = static_cast<int>(frameIndex);
            }

            ImGui::PopID();
        }

        if (removeFrameIndex >= 0
            && removeFrameIndex < static_cast<int>(animation.frames.size())) {
            animation.frames.erase(animation.frames.begin() + removeFrameIndex);

            if (animation.frames.empty()) {
                tileset.animations.erase(animationOwnerTileId);
            }
        }
    }

    if (ImGui::Button("Save Tileset Animations")) {
        saveTilesetAnimations(tileset, statusMessage);
    }
}

void SpritePalettePanel::drawLevelDropTarget(
    const ViewportGrid& viewportGrid,
    const RenderRect& viewport,
    float toolbarHeight,
    std::string& statusMessage
) {
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload == nullptr || !activePayload->IsDataType(EditorTileDrag::PayloadType)) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 displaySize = io.DisplaySize;
    if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##LevelDropTarget", nullptr, flags);
    ImGui::InvisibleButton("##LevelDropArea", displaySize);

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorTileDrag::PayloadType);
        if (payload != nullptr && payload->IsDelivery() && payload->DataSize == sizeof(TileDragPayload)) {
            const auto* tilePayload = static_cast<const TileDragPayload*>(payload->Data);
            Vector2F dropPosition = Renderer::getInstance().getCamera().screenToWorld(
                Vector2F(io.MousePos.x, io.MousePos.y),
                viewport
            );
            if (viewportGrid.shouldSnapToGrid()) {
                dropPosition = viewportGrid.snapPosition(dropPosition);
            }

            createSpriteEntityFromTile(
                tilePayload->tilesetIndex,
                tilePayload->tileId,
                dropPosition.x,
                dropPosition.y,
                statusMessage
            );
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(12.0f, toolbarHeight + 8.0f),
        IM_COL32(255, 255, 255, 220),
        "Drop tile to create a sprite entity"
    );
}

void SpritePalettePanel::createSpriteEntityFromTile(
    int tilesetIndex,
    int tileId,
    float x,
    float y,
    std::string& statusMessage
) {
    if (tilesetIndex < 0 || tilesetIndex >= static_cast<int>(tilesets.size())) {
        statusMessage = "Failed to create sprite entity: invalid tileset.";
        return;
    }

    const EditorTileset& tileset = tilesets[static_cast<std::size_t>(tilesetIndex)];
    if (tileId < 0 || tileId >= tileset.tileCount) {
        statusMessage = "Failed to create sprite entity: invalid tile id.";
        return;
    }

    if (tileset.textureAsset == nullptr || tileset.textureAsset->getTextureHandle() == nullptr) {
        statusMessage = "Failed to create sprite entity: missing texture.";
        return;
    }

    if (tileset.columns <= 0) {
        statusMessage = "Failed to create sprite entity: invalid tileset columns.";
        return;
    }

    const float renderScale = Renderer::getInstance().getRenderScale();

    const auto createSpriteFromTile = [&tileset, renderScale](int frameTileId) {
        const int column = frameTileId % tileset.columns;
        const int row = frameTileId / tileset.columns;
        const float sourceX = static_cast<float>(column * tileset.tileWidth);
        const float sourceY = static_cast<float>(row * tileset.tileHeight);

        Sprite sprite(
            tileset.textureAsset->getTextureHandle(),
            RenderRect{
                sourceX,
                sourceY,
                static_cast<float>(tileset.tileWidth),
                static_cast<float>(tileset.tileHeight)
            }
        );
        sprite.setSize(Vector2F(
            static_cast<float>(tileset.tileWidth) * renderScale,
            static_cast<float>(tileset.tileHeight) * renderScale
        ));
        return sprite;
    };

    Entity& entity = Level::getCurrentLevel().createEntity();
    entity.addComponent<TransformComponent>(Vector2F(x, y), 0.0f);

    const auto animationIterator = tileset.animations.find(tileId);
    if (animationIterator != tileset.animations.end()
        && !animationIterator->second.frames.empty()) {
        Animation animation;
        bool addedFrame = false;

        for (const EditorAnimationFrame& frame : animationIterator->second.frames) {
            if (frame.tileId < 0 || frame.tileId >= tileset.tileCount) {
                continue;
            }

            animation.addFrame(createSpriteFromTile(frame.tileId));
            if (!addedFrame) {
                animation.setFrameDuration(static_cast<float>(std::max(1, frame.durationMs)) / 1000.0f);
            }

            addedFrame = true;
        }

        if (!addedFrame) {
            statusMessage = "Failed to create animation entity: animation has no valid frames.";
            Level::getCurrentLevel().destroyEntity(entity);
            return;
        }

        entity.setName("EditorAnimation_" + std::to_string(createdSpriteCount));
        entity.setTag("EditorAnimation");
        entity.addComponent<AnimationComponent>(animation);
        statusMessage =
            "Created animation entity from owner tile id "
            + std::to_string(tileId)
            + " with " + std::to_string(animation.getFrameCount()) + " frame(s).";
    } else {
        Sprite sprite = createSpriteFromTile(tileId);
        entity.setName("EditorSprite_" + std::to_string(createdSpriteCount));
        entity.setTag("EditorSprite");
        entity.addComponent<SpriteComponent>(sprite);
        statusMessage = "Created sprite entity from tile id " + std::to_string(tileId) + ".";
    }

    ++createdSpriteCount;
    selectedTileId = tileId;
}

bool SpritePalettePanel::saveTilesetAnimations(EditorTileset& tileset, std::string& statusMessage) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(tileset.path.c_str()) != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to load tileset for saving: " + std::string(document.ErrorStr());
        return false;
    }

    tinyxml2::XMLElement* tilesetRoot = document.FirstChildElement("tileset");
    if (tilesetRoot == nullptr) {
        statusMessage = "Failed to save animations: tileset root is missing.";
        return false;
    }

    for (tinyxml2::XMLElement* tile = tilesetRoot->FirstChildElement("tile");
         tile != nullptr;
         tile = tile->NextSiblingElement("tile")) {
        for (tinyxml2::XMLElement* animation = tile->FirstChildElement("animation");
             animation != nullptr;) {
            tinyxml2::XMLElement* nextAnimation = animation->NextSiblingElement("animation");
            tile->DeleteChild(animation);
            animation = nextAnimation;
        }
    }

    std::vector<int> ownerTileIds;
    ownerTileIds.reserve(tileset.animations.size());
    for (const auto& animationPair : tileset.animations) {
        if (!animationPair.second.frames.empty()) {
            ownerTileIds.push_back(animationPair.first);
        }
    }

    std::sort(ownerTileIds.begin(), ownerTileIds.end());

    for (int ownerTileId : ownerTileIds) {
        if (ownerTileId < 0 || ownerTileId >= tileset.tileCount) {
            continue;
        }

        tinyxml2::XMLElement* tile = findTileElementById(tilesetRoot, ownerTileId);
        if (tile == nullptr) {
            tile = document.NewElement("tile");
            tile->SetAttribute("id", ownerTileId);
            tilesetRoot->InsertEndChild(tile);
        }

        tinyxml2::XMLElement* animation = document.NewElement("animation");
        const EditorTileAnimation& editorAnimation = tileset.animations[ownerTileId];

        for (const EditorAnimationFrame& editorFrame : editorAnimation.frames) {
            tinyxml2::XMLElement* frame = document.NewElement("frame");
            frame->SetAttribute("tileid", std::clamp(editorFrame.tileId, 0, std::max(0, tileset.tileCount - 1)));
            frame->SetAttribute("duration", std::max(1, editorFrame.durationMs));
            animation->InsertEndChild(frame);
        }

        tile->InsertEndChild(animation);
    }

    const tinyxml2::XMLError result = document.SaveFile(tileset.path.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to save tileset animations: " + std::string(document.ErrorStr());
        return false;
    }

    statusMessage = "Saved tileset animations to " + std::filesystem::path(tileset.path).filename().string() + ".";
    return true;
}
