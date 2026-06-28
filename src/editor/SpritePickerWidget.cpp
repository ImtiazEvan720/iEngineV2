#include "editor/SpritePickerWidget.h"

#include "editor/EditorCollectionViews.h"
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
#include <vector>

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

int getIntAttributeWithFallback(
    const tinyxml2::XMLElement* element,
    const char* primaryName,
    const char* fallbackName,
    int fallback = 0
) {
    if (element == nullptr) {
        return fallback;
    }

    if (element->FindAttribute(primaryName) != nullptr) {
        return element->IntAttribute(primaryName, fallback);
    }

    return element->IntAttribute(fallbackName, fallback);
}

std::filesystem::path resolveRelativePath(const std::filesystem::path& baseFile, const std::string& relativePath) {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return (baseFile.parent_path() / path).lexically_normal();
}

std::filesystem::path findAssetsRoot() {
    const std::filesystem::path projectAssetsPath = ProjectManager::getInstance().getAssetsPath();
    if (!projectAssetsPath.empty()
        && std::filesystem::exists(projectAssetsPath)
        && std::filesystem::is_directory(projectAssetsPath)) {
        return projectAssetsPath;
    }

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

std::vector<unsigned char> makeTileDragPayloadBytes(const TileDragPayload& payload) {
    const auto* begin = reinterpret_cast<const unsigned char*>(&payload);
    return std::vector<unsigned char>(begin, begin + sizeof(TileDragPayload));
}
}

void SpritePickerWidget::draw(std::string& statusMessage) {
    if (!tilesetsScanned) {
        refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets")) {
        refreshTilesets();
    }

    drawTilesetSelector();

    if (drawSelectedTilesetDetails(statusMessage)) {
        drawTileGrid("##SpritePickerTilesetGrid", true);
    }
}

void SpritePickerWidget::refreshTilesets() {
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

        if (toLower(entry.path().extension().string()) == ".itile") {
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

        const tinyxml2::XMLElement* tilesetRoot = document.FirstChildElement("itile");
        if (tilesetRoot == nullptr) {
            std::cerr << "Editor tileset is missing root: " << tilesetPath.string() << std::endl;
            continue;
        }

        EditorTileset tileset;
        tileset.path = tilesetPath.string();
        tileset.name = getAttribute(tilesetRoot, "name", tilesetPath.stem().string());
        tileset.tileWidth = getIntAttributeWithFallback(tilesetRoot, "tileWidth", "tilewidth");
        tileset.tileHeight = getIntAttributeWithFallback(tilesetRoot, "tileHeight", "tileheight");
        tileset.columns = tilesetRoot->IntAttribute("columns");
        tileset.tileCount = getIntAttributeWithFallback(tilesetRoot, "tileCount", "tilecount");

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
    }

    tilesetsScanned = true;
}

bool SpritePickerWidget::drawTilesetSelector() {
    if (tilesets.empty()) {
        ImGui::TextUnformatted("No .itile files found in Assets.");
        return false;
    }

    if (selectedTilesetIndex < 0 || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        selectedTilesetIndex = 0;
    }

    bool changed = false;
    const EditorTileset& selectedTileset = tilesets[static_cast<std::size_t>(selectedTilesetIndex)];
    const char* previewText = selectedTileset.name.empty() ? selectedTileset.path.c_str() : selectedTileset.name.c_str();

    if (ImGui::BeginCombo("Tileset", previewText)) {
        for (std::size_t index = 0; index < tilesets.size(); ++index) {
            const bool isSelected = selectedTilesetIndex == static_cast<int>(index);
            const std::string& label = tilesets[index].name.empty() ? tilesets[index].path : tilesets[index].name;

            if (ImGui::Selectable(label.c_str(), isSelected)) {
                selectedTilesetIndex = static_cast<int>(index);
                selectedTileId = -1;
                changed = true;
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    return changed;
}

bool SpritePickerWidget::drawSelectedTilesetDetails(std::string& statusMessage) {
    EditorTileset* tileset = getSelectedTileset();
    if (tileset == nullptr) {
        return false;
    }

    ImGui::Text("File: %s", std::filesystem::path(tileset->path).filename().string().c_str());
    ImGui::Text("Image: %s", tileset->imageFilename.empty() ? "none" : tileset->imageFilename.c_str());
    ImGui::Text("Tile size: %dx%d, tiles: %d, columns: %d",
                tileset->tileWidth,
                tileset->tileHeight,
                tileset->tileCount,
                tileset->columns);

    if (!isSelectedTilesetUsable()) {
        if (tileset->textureAsset == nullptr || tileset->textureAsset->getImGuiTextureId() == ImTextureID{}) {
            ImGui::TextUnformatted("Texture asset is not loaded for this tileset.");
        } else {
            ImGui::TextUnformatted("Tileset metadata is incomplete.");
        }

        return false;
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

    return true;
}

bool SpritePickerWidget::drawTileGrid(
    const char* childId,
    bool allowDragDrop,
    int animationOwnerTileId,
    float height,
    bool animatedTilesOnly
) {
    EditorTileset* tileset = getSelectedTileset();
    if (tileset == nullptr || !isSelectedTilesetUsable()) {
        return false;
    }

    bool changed = false;
    const ImTextureID textureId = tileset->textureAsset->getImGuiTextureId();
    const ImVec2 previewSize(
        static_cast<float>(tileset->tileWidth) * tilePreviewScale,
        static_cast<float>(tileset->tileHeight) * tilePreviewScale
    );

    std::vector<GridViewItem> items;
    std::vector<int> tileIds;
    items.reserve(static_cast<std::size_t>(tileset->tileCount));
    tileIds.reserve(static_cast<std::size_t>(tileset->tileCount));
    int selectedGridIndex = -1;

    for (int tileId = 0; tileId < tileset->tileCount; ++tileId) {
        const auto animationIterator = tileset->animations.find(tileId);
        const bool animatedOwner =
            animationIterator != tileset->animations.end()
            && !animationIterator->second.frames.empty();

        if (animatedTilesOnly && !animatedOwner) {
            continue;
        }

        const int column = tileId % tileset->columns;
        const int row = tileId / tileset->columns;
        const float sourceX = static_cast<float>(column * tileset->tileWidth);
        const float sourceY = static_cast<float>(row * tileset->tileHeight);

        GridViewItem item;
        item.id = "tile_" + std::to_string(tileId);
        item.label = std::to_string(tileId);
        item.image = textureId;
        item.imageSize = previewSize;
        item.uv0 = ImVec2(
            sourceX / static_cast<float>(tileset->imageWidth),
            sourceY / static_cast<float>(tileset->imageHeight)
        );
        item.uv1 = ImVec2(
            (sourceX + static_cast<float>(tileset->tileWidth)) / static_cast<float>(tileset->imageWidth),
            (sourceY + static_cast<float>(tileset->tileHeight)) / static_cast<float>(tileset->imageHeight)
        );

        const bool activeOwner = animationOwnerTileId == tileId;
        if (animatedOwner || activeOwner) {
            item.badge = animatedOwner ? "OWNER" : "OWNER*";
            item.badgeColor = animatedOwner
                ? IM_COL32(33, 120, 64, 220)
                : IM_COL32(120, 90, 28, 210);
        }

        if (animatedOwner) {
            item.tooltip =
                "tile id: " + std::to_string(tileId)
                + "\nanimation owner\nframes: "
                + std::to_string(animationIterator->second.frames.size());
        } else {
            item.tooltip = "tile id: " + std::to_string(tileId);
        }

        if (allowDragDrop) {
            const TileDragPayload payload{selectedTilesetIndex, tileId};
            item.dragPayloadType = EditorTileDrag::PayloadType;
            item.dragPayload = makeTileDragPayloadBytes(payload);
            item.dragLabel = "Tile id: " + std::to_string(tileId);

            Renderer& renderer = Renderer::getInstance();
            const float renderScale = renderer.getRenderScale();
            const float cameraZoom = renderer.getCamera().getZoom();
            item.dragPreviewSize = ImVec2(
                tileset->tileWidth * renderScale * cameraZoom,
                tileset->tileHeight * renderScale * cameraZoom
            );
        }

        if (selectedTileId == tileId) {
            selectedGridIndex = static_cast<int>(items.size());
        }

        tileIds.push_back(tileId);
        items.push_back(std::move(item));
    }

    GridViewOptions options;
    options.height = height;
    options.cellWidth = std::max(48.0f, previewSize.x + 16.0f);
    options.cellHeight = previewSize.y + ImGui::GetTextLineHeightWithSpacing() + 12.0f;
    options.columns = tileset->columns;
    options.horizontalScrollbar = true;
    options.emptyText = "No animated tiles found in this tileset.";

    const GridViewResult result = EditorCollectionViews::drawGridView(
        childId,
        items,
        selectedGridIndex,
        options
    );
    if (result.selectionChanged
        && result.clickedIndex >= 0
        && result.clickedIndex < static_cast<int>(tileIds.size())) {
        selectedTileId = tileIds[static_cast<std::size_t>(result.clickedIndex)];
        changed = true;
    }

    return changed;
}

std::optional<Sprite> SpritePickerWidget::createSpriteFromSelectedTile(
    float renderScale,
    std::string& statusMessage
) const {
    return createSpriteFromTile(selectedTilesetIndex, selectedTileId, renderScale, statusMessage);
}

std::optional<Sprite> SpritePickerWidget::createSpriteFromTile(
    int tilesetIndex,
    int tileId,
    float renderScale,
    std::string& statusMessage
) const {
    const EditorTileset* tileset = getTileset(tilesetIndex);
    if (tileset == nullptr) {
        statusMessage = "Failed to create sprite: invalid tileset.";
        return std::nullopt;
    }

    if (tileId < 0 || tileId >= tileset->tileCount) {
        statusMessage = "Failed to create sprite: invalid tile id.";
        return std::nullopt;
    }

    if (tileset->textureAsset == nullptr || tileset->textureAsset->getTextureHandle() == nullptr) {
        statusMessage = "Failed to create sprite: missing texture.";
        return std::nullopt;
    }

    if (tileset->columns <= 0) {
        statusMessage = "Failed to create sprite: invalid tileset columns.";
        return std::nullopt;
    }

    const int column = tileId % tileset->columns;
    const int row = tileId / tileset->columns;
    const float sourceX = static_cast<float>(column * tileset->tileWidth);
    const float sourceY = static_cast<float>(row * tileset->tileHeight);

    Sprite sprite(
        tileset->textureAsset->getTextureHandle(),
        RenderRect{
            sourceX,
            sourceY,
            static_cast<float>(tileset->tileWidth),
            static_cast<float>(tileset->tileHeight)
        }
    );
    sprite.setSize(Vector2F(
        static_cast<float>(tileset->tileWidth) * renderScale,
        static_cast<float>(tileset->tileHeight) * renderScale
    ));

    return sprite;
}

std::optional<Animation> SpritePickerWidget::createAnimationFromSelectedTile(
    float renderScale,
    std::string& statusMessage
) const {
    return createAnimationFromTile(selectedTilesetIndex, selectedTileId, renderScale, statusMessage);
}

std::optional<Animation> SpritePickerWidget::createAnimationFromTile(
    int tilesetIndex,
    int ownerTileId,
    float renderScale,
    std::string& statusMessage
) const {
    const EditorTileset* tileset = getTileset(tilesetIndex);
    if (tileset == nullptr) {
        statusMessage = "Failed to create animation: invalid tileset.";
        return std::nullopt;
    }

    if (ownerTileId < 0 || ownerTileId >= tileset->tileCount) {
        statusMessage = "Failed to create animation: invalid owner tile id.";
        return std::nullopt;
    }

    const auto animationIterator = tileset->animations.find(ownerTileId);
    if (animationIterator == tileset->animations.end()
        || animationIterator->second.frames.empty()) {
        statusMessage = "Selected tile has no animation frames.";
        return std::nullopt;
    }

    Animation animation;
    bool addedFrame = false;

    for (const EditorAnimationFrame& frame : animationIterator->second.frames) {
        std::optional<Sprite> sprite = createSpriteFromTile(
            tilesetIndex,
            frame.tileId,
            renderScale,
            statusMessage
        );

        if (!sprite.has_value()) {
            continue;
        }

        animation.addFrame(*sprite);
        if (!addedFrame) {
            animation.setFrameDuration(static_cast<float>(std::max(1, frame.durationMs)) / 1000.0f);
        }

        addedFrame = true;
    }

    if (!addedFrame) {
        statusMessage = "Failed to create animation: no valid frames.";
        return std::nullopt;
    }

    return animation;
}

bool SpritePickerWidget::hasScannedTilesets() const {
    return tilesetsScanned;
}

int SpritePickerWidget::getSelectedTilesetIndex() const {
    return selectedTilesetIndex;
}

int SpritePickerWidget::getSelectedTileId() const {
    return selectedTileId;
}

bool SpritePickerWidget::setSelectedTilesetByFilename(const std::string& filename) {
    if (!tilesetsScanned) {
        refreshTilesets();
    }

    if (filename.empty()) {
        return false;
    }

    const std::filesystem::path targetPath(filename);
    const std::string targetFilename = targetPath.filename().string();

    for (std::size_t index = 0; index < tilesets.size(); ++index) {
        const std::filesystem::path tilesetPath(tilesets[index].path);
        if (tilesetPath.filename().string() == targetFilename) {
            selectedTilesetIndex = static_cast<int>(index);
            selectedTileId = -1;
            return true;
        }
    }

    return false;
}

void SpritePickerWidget::setSelectedTileId(int tileId) {
    selectedTileId = tileId;
}

EditorTileset* SpritePickerWidget::getSelectedTileset() {
    if (selectedTilesetIndex < 0 || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        return nullptr;
    }

    return &tilesets[static_cast<std::size_t>(selectedTilesetIndex)];
}

const EditorTileset* SpritePickerWidget::getSelectedTileset() const {
    if (selectedTilesetIndex < 0 || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        return nullptr;
    }

    return &tilesets[static_cast<std::size_t>(selectedTilesetIndex)];
}

EditorTileset* SpritePickerWidget::getTileset(int tilesetIndex) {
    if (tilesetIndex < 0 || tilesetIndex >= static_cast<int>(tilesets.size())) {
        return nullptr;
    }

    return &tilesets[static_cast<std::size_t>(tilesetIndex)];
}

const EditorTileset* SpritePickerWidget::getTileset(int tilesetIndex) const {
    if (tilesetIndex < 0 || tilesetIndex >= static_cast<int>(tilesets.size())) {
        return nullptr;
    }

    return &tilesets[static_cast<std::size_t>(tilesetIndex)];
}

std::vector<EditorTileset>& SpritePickerWidget::getTilesets() {
    return tilesets;
}

const std::vector<EditorTileset>& SpritePickerWidget::getTilesets() const {
    return tilesets;
}

bool SpritePickerWidget::isSelectedTilesetUsable() const {
    const EditorTileset* tileset = getSelectedTileset();
    if (tileset == nullptr) {
        return false;
    }

    return tileset->textureAsset != nullptr
        && tileset->textureAsset->getImGuiTextureId() != ImTextureID{}
        && tileset->tileWidth > 0
        && tileset->tileHeight > 0
        && tileset->tileCount > 0
        && tileset->columns > 0
        && tileset->imageWidth > 0
        && tileset->imageHeight > 0;
}
