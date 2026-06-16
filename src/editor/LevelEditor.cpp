#include "editor/LevelEditor.h"

#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/InputSystem.h"

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

    return "Assets";
}
}

void LevelEditor::draw(const InputSystem& inputSystem, float windowWidth) {
    (void)windowWidth;

    if (ImGui::BeginMainMenuBar()) {
        if (ImGui::BeginMenu("Level")) {
            if (ImGui::MenuItem("Create")) {
                // Create a new level once editor level management is added.
            }

            if (ImGui::MenuItem("Save")) {
                // Save .ilevel data once editor serialization is added.
            }

            if (ImGui::MenuItem("Delete")) {
                // Delete/close the active level once level management is added.
            }

            if (ImGui::MenuItem("Reload")) {
                // Reload the active .tmx/.ilevel once editor loading is added.
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity")) {
            if (ImGui::MenuItem("Create Empty")) {
                // Create an entity in the current level once editor commands are added.
            }

            if (ImGui::MenuItem("Delete Selected")) {
                // Delete the selected entity once selection is added.
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Editor Enabled", nullptr, &enabled);
            ImGui::Separator();

            if (ImGui::MenuItem("Select", nullptr, currentTool == Tool::Select, enabled)) {
                currentTool = Tool::Select;
            }

            if (ImGui::MenuItem("Move", nullptr, currentTool == Tool::Move, enabled)) {
                currentTool = Tool::Move;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            ImGui::MenuItem("Show Grid", nullptr, &showGrid);
            ImGui::MenuItem("Show Colliders", nullptr, &showColliders);

            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    ImGui::Begin("Level Editor");

    if (ImGui::BeginTabBar("##LevelEditorTabs")) {
        if (ImGui::BeginTabItem("Level_Outline")) {
            drawLevelOutlineTab(inputSystem);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Sprites")) {
            drawSpritesTab();
            ImGui::EndTabItem();
        }
        
        if (ImGui::BeginTabItem("File_Explorer")) {
            drawFileExplorerTab();
            ImGui::EndTabItem();
        }

        ImGui::EndTabBar();
    }

    ImGui::End();
}

void LevelEditor::setEnabled(bool value) {
    enabled = value;
}

bool LevelEditor::isEnabled() const {
    return enabled;
}

void LevelEditor::toggleEnabled() {
    enabled = !enabled;
}

LevelEditor::Tool LevelEditor::getCurrentTool() const {
    return currentTool;
}

float LevelEditor::getToolbarHeight() const {
    if (ImGui::GetCurrentContext() == nullptr) {
        return 0.0f;
    }

    return ImGui::GetFrameHeight();
}

bool LevelEditor::shouldShowGrid() const {
    return showGrid;
}

bool LevelEditor::shouldShowColliders() const {
    return showColliders;
}

void LevelEditor::drawLevelOutlineTab(const InputSystem& inputSystem) {
    ImGui::Text("Input: %s", inputSystem.getLastInputText().c_str());
    ImGui::Text("Editor: %s", enabled ? "Enabled" : "Disabled");
    ImGui::Text("Tool: %s", currentTool == Tool::Select ? "Select" : "Move");
}

void LevelEditor::drawSpritesTab() {
    if (!tilesetsScanned) {
        refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets")) {
        refreshTilesets();
    }

    drawTilesetSelector();
    drawSelectedTilesetGrid();
}

void LevelEditor::drawFileExplorerTab() {
    ImGui::Text("File explorer goes here.");
}

void LevelEditor::refreshTilesets() {
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

        tilesets.push_back(tileset);
    }

    if (!tilesets.empty()) {
        selectedTilesetIndex = 0;
    }

    tilesetsScanned = true;
}

void LevelEditor::drawTilesetSelector() {
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
            }

            if (isSelected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }
}

void LevelEditor::drawSelectedTilesetGrid() {
    if (tilesets.empty()
        || selectedTilesetIndex < 0
        || selectedTilesetIndex >= static_cast<int>(tilesets.size())) {
        return;
    }

    const EditorTileset& tileset = tilesets[static_cast<std::size_t>(selectedTilesetIndex)];

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

            if (selected) {
                ImGui::PopStyleColor();
            }

            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("tile id: %d", tileId);
            }

            ImGui::PopID();

            if ((tileId + 1) % tileset.columns != 0) {
                ImGui::SameLine();
            }
        }
    }

    ImGui::EndChild();
}
