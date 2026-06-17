#include "editor/LevelEditor.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/InputSystem.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tinyxml2.h"

#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <system_error>

namespace {
constexpr const char* TileDragPayloadType = "IENGINE_TILE";

struct TileDragPayload {
    int tilesetIndex = -1;
    int tileId = -1;
};

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

std::vector<TextureAsset*> getLoadedTextureAssets() {
    std::vector<TextureAsset*> textureAssets;

    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        auto* textureAsset = dynamic_cast<TextureAsset*>(asset.get());
        if (textureAsset != nullptr && textureAsset->isLoaded()) {
            textureAssets.push_back(textureAsset);
        }
    }

    return textureAssets;
}

std::string makeDefaultTilesetName(const TextureAsset& textureAsset) {
    std::filesystem::path texturePath(textureAsset.getPath());
    return texturePath.stem().string();
}

std::filesystem::path resolveTilesetOutputPath(const std::string& outputFile) {
    std::filesystem::path outputPath(outputFile);

    if (outputPath.empty()) {
        outputPath = "NewTileset.tsx";
    }

    if (outputPath.extension() != ".tsx") {
        outputPath.replace_extension(".tsx");
    }

    if (!outputPath.is_absolute()) {
        const std::filesystem::path assetsRoot = findAssetsRoot();
        const auto firstPart = outputPath.begin();
        if (firstPart == outputPath.end() || *firstPart != assetsRoot.filename()) {
            outputPath = assetsRoot / outputPath;
        }
    }

    return outputPath.lexically_normal();
}

int colorFloatToByte(float value) {
    return std::clamp(static_cast<int>(std::round(value * 255.0f)), 0, 255);
}

void appendHexByte(std::string& output, int byte) {
    constexpr char HexDigits[] = "0123456789abcdef";
    const int clampedByte = std::clamp(byte, 0, 255);
    output.push_back(HexDigits[(clampedByte >> 4) & 0x0f]);
    output.push_back(HexDigits[clampedByte & 0x0f]);
}

std::string colorBytesToTiledHex(int red, int green, int blue) {
    std::string result;
    result.reserve(6);
    appendHexByte(result, red);
    appendHexByte(result, green);
    appendHexByte(result, blue);
    return result;
}

std::string colorToTiledHex(const float color[3]) {
    return colorBytesToTiledHex(
        colorFloatToByte(color[0]),
        colorFloatToByte(color[1]),
        colorFloatToByte(color[2])
    );
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

int bodyTypeToIndex(CollisionComponent::BodyType bodyType) {
    switch (bodyType) {
        case CollisionComponent::BodyType::Kinematic:
            return 1;
        case CollisionComponent::BodyType::Dynamic:
            return 2;
        case CollisionComponent::BodyType::Static:
        default:
            return 0;
    }
}

CollisionComponent::BodyType bodyTypeFromIndex(int index) {
    switch (index) {
        case 1:
            return CollisionComponent::BodyType::Kinematic;
        case 2:
            return CollisionComponent::BodyType::Dynamic;
        case 0:
        default:
            return CollisionComponent::BodyType::Static;
    }
}

float getEditorGridSize() {
    return 16.0f * Renderer::getInstance().getRenderScale();
}

Vector2F snapPositionToGrid(const Vector2F& position) {
    const float gridSize = getEditorGridSize();
    if (gridSize <= 0.0f) {
        return position;
    }

    const float halfGridSize = gridSize * 0.5f;
    return Vector2F(
        std::round((position.x - halfGridSize) / gridSize) * gridSize + halfGridSize,
        std::round((position.y - halfGridSize) / gridSize) * gridSize + halfGridSize
    );
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

        drawAssetsMenu();

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
            ImGui::MenuItem("Snap to Grid", nullptr, &snapToGrid);
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    drawCreateTilesetPopup();

    ImGui::Begin("Level Editor");

    if (ImGui::BeginTabBar("##LevelEditorTabs")) {
        if (ImGui::BeginTabItem("Level_Outline")) {
            drawLevelOutlineTab(inputSystem);
            ImGui::EndTabItem();
        }

        if (ImGui::BeginTabItem("Entities")) {
            drawEntitiesTab();
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

    consumedPinchZoomThisFrame = false;
    handleViewportCameraZoom();
    handleViewportCameraPan();

    if (showGrid) {
        drawViewportGrid();
    }

    handleViewportEntityInteraction();
    drawLevelDropTarget();
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

bool LevelEditor::shouldSnapToGrid() const {
    return snapToGrid;
}

void LevelEditor::drawLevelOutlineTab(const InputSystem& inputSystem) {
    ImGui::Text("Input: %s", inputSystem.getLastInputText().c_str());
    ImGui::Text("Editor: %s", enabled ? "Enabled" : "Disabled");
    ImGui::Text("Tool: %s", currentTool == Tool::Select ? "Select" : "Move");
}

void LevelEditor::drawEntitiesTab() {
    Level& level = Level::getCurrentLevel();
    auto& entities = level.getEntities();

    ImGui::Text("Entities: %zu", entities.size());

    if (selectedEntityId >= 0) {
        ImGui::SameLine();
        ImGui::Text("Selected: %d", selectedEntityId);
    }

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::Separator();

    if (entities.empty()) {
        ImGui::TextUnformatted("Current level has no entities.");
        return;
    }

    if (ImGui::BeginChild(
            "##EntityTree",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        const bool levelOpen = ImGui::TreeNodeEx(
            "Current Level",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth
        );

        if (levelOpen) {
            for (Entity& entity : entities) {
                drawEntityTreeNode(entity);
            }

            ImGui::TreePop();
        }
    }

    ImGui::EndChild();
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

void LevelEditor::drawAssetsMenu() {
    if (!ImGui::BeginMenu("Assets")) {
        return;
    }

    if (ImGui::MenuItem("Create Tileset")) {
        resetCreateTilesetForm();
        showCreateTilesetPopup = true;
    }

    if (ImGui::MenuItem("Refresh Tilesets")) {
        refreshTilesets();
        statusMessage = "Refreshed tilesets.";
    }

    ImGui::EndMenu();
}

void LevelEditor::drawCreateTilesetPopup() {
    if (showCreateTilesetPopup) {
        ImGui::OpenPopup("Create Tileset");
        showCreateTilesetPopup = false;
    }

    if (!ImGui::BeginPopupModal("Create Tileset", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    std::vector<TextureAsset*> textureAssets = getLoadedTextureAssets();
    if (textureAssets.empty()) {
        ImGui::TextUnformatted("No loaded texture assets found.");
        if (ImGui::Button("Close")) {
            ImGui::CloseCurrentPopup();
        }

        ImGui::EndPopup();
        return;
    }

    if (selectedCreateTilesetTextureIndex < 0
        || selectedCreateTilesetTextureIndex >= static_cast<int>(textureAssets.size())) {
        selectedCreateTilesetTextureIndex = 0;
    }

    TextureAsset* selectedTexture = textureAssets[static_cast<std::size_t>(selectedCreateTilesetTextureIndex)];
    if (selectedTexture != nullptr) {
        const std::string defaultName = makeDefaultTilesetName(*selectedTexture);
        if (newTilesetName.empty()) {
            newTilesetName = defaultName;
        }

        if (newTilesetOutputFile.empty()) {
            newTilesetOutputFile = defaultName + ".tsx";
        }
    }

    const std::string previewText = selectedTexture == nullptr
        ? "None"
        : std::filesystem::path(selectedTexture->getPath()).filename().string();

    if (ImGui::BeginCombo("Texture", previewText.c_str())) {
        for (std::size_t index = 0; index < textureAssets.size(); ++index) {
            TextureAsset* textureAsset = textureAssets[index];
            const std::string label = std::filesystem::path(textureAsset->getPath()).filename().string();
            const bool selected = selectedCreateTilesetTextureIndex == static_cast<int>(index);

            if (ImGui::Selectable(label.c_str(), selected)) {
                selectedCreateTilesetTextureIndex = static_cast<int>(index);
                resetCreateTilesetPreviewImage();

                const std::string defaultName = makeDefaultTilesetName(*textureAsset);
                newTilesetName = defaultName;
                newTilesetOutputFile = defaultName + ".tsx";
            }

            if (selected) {
                ImGui::SetItemDefaultFocus();
            }
        }

        ImGui::EndCombo();
    }

    ImGui::InputText("Tileset Name", &newTilesetName);
    ImGui::InputInt("Tile Width", &newTilesetTileWidth);
    ImGui::InputInt("Tile Height", &newTilesetTileHeight);
    ImGui::InputText("Output File", &newTilesetOutputFile);
    ImGui::TextWrapped("Save path: %s", resolveTilesetOutputPath(newTilesetOutputFile).string().c_str());

    ImGui::Checkbox("Use Transparency Color", &newTilesetUseTransparencyColor);
    if (!newTilesetUseTransparencyColor) {
        pickingTransparencyColor = false;
    }

    if (!newTilesetUseTransparencyColor) {
        ImGui::BeginDisabled();
    }

    ImGui::ColorEdit3(
        "Transparency Color",
        newTilesetTransparencyColor,
        ImGuiColorEditFlags_DisplayHex | ImGuiColorEditFlags_NoAlpha
    );

    if (ImGui::Button(pickingTransparencyColor ? "Cancel Eyedropper" : "Pick From Image")) {
        pickingTransparencyColor = !pickingTransparencyColor;
    }

    if (pickingTransparencyColor) {
        ImGui::SameLine();
        ImGui::TextUnformatted("Click a pixel in the preview.");
    }

    if (!newTilesetUseTransparencyColor) {
        ImGui::EndDisabled();
    }

    if (newTilesetTileWidth < 1) {
        newTilesetTileWidth = 1;
    }

    if (newTilesetTileHeight < 1) {
        newTilesetTileHeight = 1;
    }

    int columns = 0;
    int rows = 0;
    int tileCount = 0;
    bool validTileGrid = false;

    if (selectedTexture != nullptr) {
        const int imageWidth = selectedTexture->getWidth();
        const int imageHeight = selectedTexture->getHeight();
        validTileGrid =
            imageWidth > 0
            && imageHeight > 0
            && newTilesetTileWidth > 0
            && newTilesetTileHeight > 0
            && imageWidth % newTilesetTileWidth == 0
            && imageHeight % newTilesetTileHeight == 0;

        if (validTileGrid) {
            columns = imageWidth / newTilesetTileWidth;
            rows = imageHeight / newTilesetTileHeight;
            tileCount = columns * rows;
        }

        ImGui::Text("Image size: %dx%d", imageWidth, imageHeight);
        ImGui::Text("Generated grid: %d columns x %d rows, %d tiles", columns, rows, tileCount);
        drawCreateTilesetImagePreview(*selectedTexture);
    }

    if (!validTileGrid) {
        ImGui::TextUnformatted("Tile size must divide the texture width and height exactly.");
    }

    const bool canCreate =
        selectedTexture != nullptr
        && !newTilesetName.empty()
        && !newTilesetOutputFile.empty()
        && validTileGrid;

    if (!canCreate) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Create and Save")) {
        if (createTilesetFromTexture(
                *selectedTexture,
                newTilesetName,
                newTilesetTileWidth,
                newTilesetTileHeight,
                newTilesetUseTransparencyColor,
                newTilesetTransparencyColor)) {
            refreshTilesets();
            ImGui::CloseCurrentPopup();
        }
    }

    if (!canCreate) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel")) {
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

void LevelEditor::drawEntityTreeNode(Entity& entity) {
    ImGui::PushID(entity.getId());

    std::string label = entity.getName();
    if (label.empty()) {
        label = "Entity";
    }

    label += "##" + std::to_string(entity.getId());

    ImGuiTreeNodeFlags flags =
        ImGuiTreeNodeFlags_OpenOnArrow |
        ImGuiTreeNodeFlags_OpenOnDoubleClick |
        ImGuiTreeNodeFlags_SpanAvailWidth;

    if (selectedEntityId == entity.getId()) {
        flags |= ImGuiTreeNodeFlags_Selected;
    }

    const bool open = ImGui::TreeNodeEx(label.c_str(), flags);

    if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
        selectedEntityId = entity.getId();
        syncEditStateFromEntity(entity);
    }

    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip(
            "id: %d\nname: %s\ntag: %s",
            entity.getId(),
            entity.getName().c_str(),
            entity.getTag().c_str()
        );
    }

    if (open) {
        ImGui::Text("Id: %d", entity.getId());
        ImGui::Text("State: %s", entity.isDestroyed() ? "Destroyed" : "Active");

        if (selectedEntityId == entity.getId()) {
            syncEditStateFromEntity(entity);
            drawEntityIdentityFields(entity);
        } else {
            ImGui::Text("Name: %s", entity.getName().c_str());
            ImGui::Text("Tag: %s", entity.getTag().c_str());
        }

        drawEntityComponents(entity);
        ImGui::TreePop();
    }

    ImGui::PopID();
}

void LevelEditor::drawEntityComponents(Entity& entity) {
    if (!ImGui::TreeNodeEx(
            "Components",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        return;
    }

    bool hasComponents = false;
    const bool editable = selectedEntityId == entity.getId();

    if (auto* transform = entity.getComponent<TransformComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("TransformComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawTransformComponentFields(*transform);
            } else {
                const Vector2F& position = transform->getPosition();
                const Vector2F worldPosition = transform->getWorldPosition();

                ImGui::Text("Position: %.2f, %.2f", position.x, position.y);
                ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
                ImGui::Text("Rotation: %.2f", transform->getRotation());
                ImGui::Text("World Rotation: %.2f", transform->getWorldRotation());
            }

            ImGui::TreePop();
        }
    }

    if (auto* spriteComponent = entity.getComponent<SpriteComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("SpriteComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawSpriteComponentFields(*spriteComponent);
            } else {
                const Sprite& sprite = spriteComponent->getSprite();
                const RenderRect& source = sprite.getSourceRect();
                const Vector2F& size = sprite.getSize();
                const Vector2F& origin = sprite.getOrigin();

                ImGui::Text("Source: %.2f, %.2f, %.2f, %.2f", source.x, source.y, source.width, source.height);
                ImGui::Text("Size: %.2f, %.2f", size.x, size.y);
                ImGui::Text("Origin: %.2f, %.2f", origin.x, origin.y);
            }

            ImGui::TreePop();
        }
    }

    if (auto* animationComponent = entity.getComponent<AnimationComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("AnimationComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawAnimationComponentFields(*animationComponent);
            } else {
                const Animation& animation = animationComponent->getAnimation();

                ImGui::Text("Frames: %zu", animation.getFrameCount());
                ImGui::Text("Frame Duration: %.3f", animation.getFrameDuration());
                ImGui::Text("Playing: %s", animationComponent->isPlaying() ? "true" : "false");
                ImGui::Text("Finished: %s", animationComponent->isFinished() ? "true" : "false");
            }

            ImGui::TreePop();
        }
    }

    if (auto* collisionComponent = entity.getComponent<CollisionComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("CollisionComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                drawCollisionComponentFields(*collisionComponent);
            } else {
                ImGui::Text("Name: %s", collisionComponent->getName().c_str());
                ImGui::Text("Size: %.2f, %.2f", collisionComponent->getWidth(), collisionComponent->getHeight());
                ImGui::Text("Sensor: %s", collisionComponent->isSensor() ? "true" : "false");
            }

            ImGui::TreePop();
        }
    }

    if (auto* scriptComponent = entity.getComponent<ScriptComponent>()) {
        hasComponents = true;
        if (ImGui::TreeNodeEx("ScriptComponent", ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth)) {
            drawScriptComponentFields(*scriptComponent);
            ImGui::TreePop();
        }
    }

    if (entity.getComponent<PlayerController>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("PlayerController");
    }

    if (entity.getComponent<Brick>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("Brick");
    }

    if (entity.getComponent<Bullet>() != nullptr) {
        hasComponents = true;
        ImGui::BulletText("Bullet");
    }

    if (!hasComponents) {
        ImGui::TextUnformatted("No known components.");
    }

    ImGui::TreePop();
}

void LevelEditor::drawEntityIdentityFields(Entity& entity) {
    ImGui::InputText("Name", &entityEditState.name);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setName(entityEditState.name);
        statusMessage = "Updated entity name.";
    }

    ImGui::InputText("Tag", &entityEditState.tag);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setTag(entityEditState.tag);
        statusMessage = "Updated entity tag.";
    }
}

void LevelEditor::drawTransformComponentFields(TransformComponent& transform) {
    ImGui::InputFloat2("Position", entityEditState.position, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setPosition(Vector2F(entityEditState.position[0], entityEditState.position[1]));
        statusMessage = "Updated TransformComponent position.";
    }

    ImGui::InputFloat("Rotation", &entityEditState.rotation, 0.0f, 0.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setRotation(entityEditState.rotation);
        statusMessage = "Updated TransformComponent rotation.";
    }

    const Vector2F worldPosition = transform.getWorldPosition();
    ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
    ImGui::Text("World Rotation: %.2f", transform.getWorldRotation());
}

void LevelEditor::drawSpriteComponentFields(SpriteComponent& spriteComponent) {
    Sprite& sprite = spriteComponent.getSprite();

    ImGui::InputFloat4("Source Rect", entityEditState.spriteSource, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSourceRect(RenderRect{
            entityEditState.spriteSource[0],
            entityEditState.spriteSource[1],
            entityEditState.spriteSource[2],
            entityEditState.spriteSource[3]
        });
        statusMessage = "Updated SpriteComponent source rect.";
    }

    ImGui::InputFloat2("Size", entityEditState.spriteSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSize(Vector2F(entityEditState.spriteSize[0], entityEditState.spriteSize[1]));
        statusMessage = "Updated SpriteComponent size.";
    }

    ImGui::InputFloat2("Origin", entityEditState.spriteOrigin, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setOrigin(Vector2F(entityEditState.spriteOrigin[0], entityEditState.spriteOrigin[1]));
        statusMessage = "Updated SpriteComponent origin.";
    }
}

void LevelEditor::drawAnimationComponentFields(AnimationComponent& animationComponent) {
    Animation& animation = animationComponent.getAnimation();

    ImGui::Text("Frames: %zu", animation.getFrameCount());
    ImGui::Text("Finished: %s", animationComponent.isFinished() ? "true" : "false");

    ImGui::InputFloat("Frame Duration", &entityEditState.animationFrameDuration, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.animationFrameDuration = std::max(0.001f, entityEditState.animationFrameDuration);
        animation.setFrameDuration(entityEditState.animationFrameDuration);
        statusMessage = "Updated AnimationComponent frame duration.";
    }

    if (ImGui::Checkbox("Playing", &entityEditState.animationPlaying)) {
        if (entityEditState.animationPlaying) {
            animationComponent.play();
        } else {
            animationComponent.pause();
        }

        statusMessage = "Updated AnimationComponent playback.";
    }
}

void LevelEditor::drawCollisionComponentFields(CollisionComponent& collisionComponent) {
    ImGui::InputText("Name", &entityEditState.collisionName);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent.setName(entityEditState.collisionName);
        statusMessage = "Updated CollisionComponent name.";
    }

    ImGui::InputFloat2("Size", entityEditState.collisionSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.collisionSize[0] = std::max(0.001f, entityEditState.collisionSize[0]);
        entityEditState.collisionSize[1] = std::max(0.001f, entityEditState.collisionSize[1]);
        collisionComponent.setSize(entityEditState.collisionSize[0], entityEditState.collisionSize[1]);
        statusMessage = "Updated CollisionComponent size.";
    }

    if (ImGui::Combo("Body Type", &entityEditState.collisionBodyType, "Static\0Kinematic\0Dynamic\0")) {
        collisionComponent.setBodyType(bodyTypeFromIndex(entityEditState.collisionBodyType));
        statusMessage = "Updated CollisionComponent body type.";
    }

    if (ImGui::Checkbox("Sensor", &entityEditState.collisionSensor)) {
        collisionComponent.setSensor(entityEditState.collisionSensor);
        statusMessage = "Updated CollisionComponent sensor.";
    }
}

void LevelEditor::drawScriptComponentFields(ScriptComponent& scriptComponent) {
    ImGui::TextWrapped("Path: %s", scriptComponent.getScriptPath().c_str());
    ImGui::Text("Loaded: %s", scriptComponent.isLoaded() ? "true" : "false");
}

void LevelEditor::handleViewportCameraZoom() {
    if (!enabled) {
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    Renderer& renderer = Renderer::getInstance();
    const float pinchZoomFactor = renderer.consumePendingPinchZoomFactor();
    const bool pinchZoom =
        std::isfinite(pinchZoomFactor)
        && pinchZoomFactor > 0.0f
        && pinchZoomFactor != 1.0f;
    const bool zoomGesture = io.KeyCtrl || io.KeySuper;
    const bool wheelZoom = zoomGesture && io.MouseWheel != 0.0f;

    if (io.WantCaptureMouse || (!pinchZoom && !wheelZoom)) {
        return;
    }

    float zoomMultiplier = 1.0f;
    if (pinchZoom) {
        zoomMultiplier *= pinchZoomFactor;
        consumedPinchZoomThisFrame = true;
    }

    if (wheelZoom) {
        zoomMultiplier *= std::pow(1.1f, io.MouseWheel);
    }

    renderer.getCamera().zoomAtScreenPoint(
        zoomMultiplier,
        Vector2F(io.MousePos.x, io.MousePos.y),
        getViewportForCamera()
    );

    statusMessage = "Viewport zoom: " + std::to_string(renderer.getCamera().getZoom());
}

void LevelEditor::handleViewportCameraPan() {
    constexpr float WheelPanPixels = 48.0f;

    if (!enabled) {
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const bool zoomGesture = io.KeyCtrl || io.KeySuper;
    const bool wheelPan =
        !zoomGesture
        && !consumedPinchZoomThisFrame
        && (io.MouseWheel != 0.0f || io.MouseWheelH != 0.0f);
    const bool dragPan = isViewportCameraPanActive();

    if (io.WantCaptureMouse || (!dragPan && !wheelPan)) {
        return;
    }

    Camera2D& camera = Renderer::getInstance().getCamera();
    const float zoom = camera.getZoom();
    if (zoom <= 0.0f) {
        return;
    }

    Vector2F position = camera.getPosition();

    if (dragPan) {
        position.x -= io.MouseDelta.x / zoom;
        position.y -= io.MouseDelta.y / zoom;
    }

    if (wheelPan) {
        position.x -= (io.MouseWheelH * WheelPanPixels) / zoom;
        position.y -= (io.MouseWheel * WheelPanPixels) / zoom;
    }

    camera.setPosition(position);
}

bool LevelEditor::isViewportCameraPanActive() const {
    const bool middleDrag = ImGui::IsMouseDragging(ImGuiMouseButton_Middle, 0.0f);
    const bool spaceLeftDrag =
        ImGui::IsKeyDown(ImGuiKey_Space)
        && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 0.0f);

    return middleDrag || spaceLeftDrag;
}

void LevelEditor::handleViewportEntityInteraction() {
    if (!enabled) {
        draggingEntityId = -1;
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (isViewportCameraPanActive()) {
        draggingEntityId = -1;
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const RenderRect viewport = getViewportForCamera();
    Camera2D& camera = Renderer::getInstance().getCamera();
    const Vector2F worldMousePosition = camera.screenToWorld(
        Vector2F(mousePosition.x, mousePosition.y),
        viewport
    );

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        Entity* entity = findEntityAt(worldMousePosition.x, worldMousePosition.y);
        if (entity != nullptr) {
            selectedEntityId = entity->getId();
            draggingEntityId = entity->getId();
            currentTool = Tool::Move;

            if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
                const Vector2F& position = transform->getPosition();
                dragOffset[0] = worldMousePosition.x - position.x;
                dragOffset[1] = worldMousePosition.y - position.y;
            } else {
                dragOffset[0] = 0.0f;
                dragOffset[1] = 0.0f;
            }

            syncEditStateFromEntity(*entity, true);
            statusMessage = "Selected entity " + std::to_string(entity->getId()) + ".";
        } else {
            selectedEntityId = -1;
            draggingEntityId = -1;
            entityEditState = EntityEditState{};
        }
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        Entity* entity = findEntityById(draggingEntityId);
        if (entity == nullptr) {
            draggingEntityId = -1;
            return;
        }

        TransformComponent* transform = entity->getComponent<TransformComponent>();
        if (transform == nullptr) {
            draggingEntityId = -1;
            return;
        }

        Vector2F newPosition(
            worldMousePosition.x - dragOffset[0],
            worldMousePosition.y - dragOffset[1]
        );
        if (snapToGrid) {
            newPosition = snapPositionToGrid(newPosition);
        }

        transform->setPosition(newPosition);
        syncEditStateFromEntity(*entity, true);
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        Entity* entity = findEntityById(draggingEntityId);
        if (entity != nullptr) {
            syncEditStateFromEntity(*entity, true);
            statusMessage = "Moved entity " + std::to_string(entity->getId()) + ".";
        }

        draggingEntityId = -1;
    }
}

Entity* LevelEditor::findEntityAt(float x, float y) {
    auto& entities = Level::getCurrentLevel().getEntities();

    for (auto iterator = entities.rbegin(); iterator != entities.rend(); ++iterator) {
        if (iterator->isDestroyed()) {
            continue;
        }

        if (entityContainsPoint(*iterator, x, y)) {
            return &*iterator;
        }
    }

    return nullptr;
}

Entity* LevelEditor::findEntityById(int id) {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (entity.getId() == id && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}

bool LevelEditor::entityContainsPoint(Entity& entity, float x, float y) const {
    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return false;
    }

    const Sprite* sprite = nullptr;
    if (AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
        animationComponent != nullptr && animationComponent->getAnimation().hasFrames()) {
        sprite = &animationComponent->getCurrentFrame();
    } else if (SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
        sprite = &spriteComponent->getSprite();
    }

    if (sprite == nullptr) {
        return false;
    }

    const Vector2F worldPosition = transform->getWorldPosition();
    const Vector2F& size = sprite->getSize();
    const Vector2F& origin = sprite->getOrigin();

    const float left = worldPosition.x - origin.x;
    const float top = worldPosition.y - origin.y;
    const float right = left + size.x;
    const float bottom = top + size.y;

    return x >= left && x <= right && y >= top && y <= bottom;
}

RenderRect LevelEditor::getViewportForCamera() const {
    RenderRect viewport = Renderer::getInstance().getViewport();
    if (viewport.width > 0.0f && viewport.height > 0.0f) {
        return viewport;
    }

    const ImGuiIO& io = ImGui::GetIO();
    return RenderRect{
        0.0f,
        0.0f,
        io.DisplaySize.x,
        io.DisplaySize.y
    };
}

void LevelEditor::syncEditStateFromEntity(Entity& entity, bool force) {
    if (!force && entityEditState.entityId == entity.getId()) {
        return;
    }

    entityEditState = EntityEditState{};
    entityEditState.entityId = entity.getId();
    entityEditState.name = entity.getName();
    entityEditState.tag = entity.getTag();

    if (TransformComponent* transform = entity.getComponent<TransformComponent>()) {
        const Vector2F& position = transform->getPosition();
        entityEditState.position[0] = position.x;
        entityEditState.position[1] = position.y;
        entityEditState.rotation = transform->getRotation();
    }

    if (SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
        const Sprite& sprite = spriteComponent->getSprite();
        const RenderRect& source = sprite.getSourceRect();
        const Vector2F& size = sprite.getSize();
        const Vector2F& origin = sprite.getOrigin();

        entityEditState.spriteSource[0] = source.x;
        entityEditState.spriteSource[1] = source.y;
        entityEditState.spriteSource[2] = source.width;
        entityEditState.spriteSource[3] = source.height;
        entityEditState.spriteSize[0] = size.x;
        entityEditState.spriteSize[1] = size.y;
        entityEditState.spriteOrigin[0] = origin.x;
        entityEditState.spriteOrigin[1] = origin.y;
    }

    if (AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>()) {
        entityEditState.animationFrameDuration = animationComponent->getAnimation().getFrameDuration();
        entityEditState.animationPlaying = animationComponent->isPlaying();
    }

    if (CollisionComponent* collisionComponent = entity.getComponent<CollisionComponent>()) {
        entityEditState.collisionSize[0] = collisionComponent->getWidth();
        entityEditState.collisionSize[1] = collisionComponent->getHeight();
        entityEditState.collisionBodyType = bodyTypeToIndex(collisionComponent->getBodyType());
        entityEditState.collisionSensor = collisionComponent->isSensor();
        entityEditState.collisionName = collisionComponent->getName();
    }
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
                animationOwnerTileId = -1;
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

    drawSelectedTileAnimationEditor(tileset);

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

            if (ImGui::BeginDragDropSource()) {
                const TileDragPayload payload{
                    selectedTilesetIndex,
                    tileId
                };

                ImGui::SetDragDropPayload(TileDragPayloadType, &payload, sizeof(payload));
                ImGui::Text("Tile id: %d", tileId);
                // ImGui::Image(textureId, previewSize, uv0, uv1);

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

void LevelEditor::drawSelectedTileAnimationEditor(EditorTileset& tileset) {
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
        saveTilesetAnimations(tileset);
    }
}

void LevelEditor::drawLevelDropTarget() {
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload == nullptr || !activePayload->IsDataType(TileDragPayloadType)) {
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
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(TileDragPayloadType);
        if (payload != nullptr && payload->IsDelivery() && payload->DataSize == sizeof(TileDragPayload)) {
            const auto* tilePayload = static_cast<const TileDragPayload*>(payload->Data);
            Vector2F dropPosition = Renderer::getInstance().getCamera().screenToWorld(
                Vector2F(io.MousePos.x, io.MousePos.y),
                getViewportForCamera()
            );
            if (snapToGrid) {
                dropPosition = snapPositionToGrid(dropPosition);
            }

            createSpriteEntityFromTile(
                tilePayload->tilesetIndex,
                tilePayload->tileId,
                dropPosition.x,
                dropPosition.y
            );
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(12.0f, getToolbarHeight() + 8.0f),
        IM_COL32(255, 255, 255, 220),
        "Drop tile to create a sprite entity"
    );
}

void LevelEditor::createSpriteEntityFromTile(int tilesetIndex, int tileId, float x, float y) {
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

void LevelEditor::drawCreateTilesetImagePreview(TextureAsset& textureAsset) {
    const ImTextureID textureId = textureAsset.getImGuiTextureId();
    const int imageWidth = textureAsset.getWidth();
    const int imageHeight = textureAsset.getHeight();

    if (textureId == ImTextureID{} || imageWidth <= 0 || imageHeight <= 0) {
        return;
    }

    const bool imagePixelsLoaded = loadCreateTilesetPreviewImage(textureAsset);
    const float availableWidth = std::max(1.0f, ImGui::GetContentRegionAvail().x);
    const float maxPreviewWidth = std::min(availableWidth, 420.0f);
    constexpr float MaxPreviewHeight = 220.0f;
    const float scale = std::clamp(
        std::min(
            maxPreviewWidth / static_cast<float>(imageWidth),
            MaxPreviewHeight / static_cast<float>(imageHeight)
        ),
        0.1f,
        4.0f
    );
    const ImVec2 previewSize(
        static_cast<float>(imageWidth) * scale,
        static_cast<float>(imageHeight) * scale
    );

    ImGui::Separator();
    ImGui::TextUnformatted("Image Preview");
    ImGui::Image(textureId, previewSize);

    const ImVec2 imageMin = ImGui::GetItemRectMin();
    const ImVec2 imageMax = ImGui::GetItemRectMax();

    if (pickingTransparencyColor) {
        ImGui::GetWindowDrawList()->AddRect(
            imageMin,
            imageMax,
            IM_COL32(255, 210, 64, 255),
            0.0f,
            0,
            2.0f
        );
    }

    if (!imagePixelsLoaded) {
        ImGui::TextUnformatted("Could not load image pixels for eyedropper.");
        return;
    }

    if (!ImGui::IsItemHovered()) {
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const float localX = std::clamp(mousePosition.x - imageMin.x, 0.0f, previewSize.x);
    const float localY = std::clamp(mousePosition.y - imageMin.y, 0.0f, previewSize.y);
    const int pixelX = std::clamp(
        static_cast<int>((localX / previewSize.x) * static_cast<float>(createTilesetPreviewImageWidth)),
        0,
        createTilesetPreviewImageWidth - 1
    );
    const int pixelY = std::clamp(
        static_cast<int>((localY / previewSize.y) * static_cast<float>(createTilesetPreviewImageHeight)),
        0,
        createTilesetPreviewImageHeight - 1
    );
    const std::size_t pixelIndex =
        (static_cast<std::size_t>(pixelY) * static_cast<std::size_t>(createTilesetPreviewImageWidth)
         + static_cast<std::size_t>(pixelX)) * 4U;

    if (pixelIndex + 2U >= createTilesetPreviewPixels.size()) {
        return;
    }

    const int red = createTilesetPreviewPixels[pixelIndex];
    const int green = createTilesetPreviewPixels[pixelIndex + 1U];
    const int blue = createTilesetPreviewPixels[pixelIndex + 2U];
    const std::string hexColor = colorBytesToTiledHex(red, green, blue);

    ImGui::SetTooltip("Pixel: %d, %d\nColor: #%s", pixelX, pixelY, hexColor.c_str());

    if (pickingTransparencyColor && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        newTilesetTransparencyColor[0] = static_cast<float>(red) / 255.0f;
        newTilesetTransparencyColor[1] = static_cast<float>(green) / 255.0f;
        newTilesetTransparencyColor[2] = static_cast<float>(blue) / 255.0f;
        newTilesetUseTransparencyColor = true;
        pickingTransparencyColor = false;
        statusMessage = "Picked transparency color #" + hexColor + ".";
    }
}

bool LevelEditor::loadCreateTilesetPreviewImage(TextureAsset& textureAsset) {
    const std::string imagePath = textureAsset.getPath();
    if (createTilesetPreviewImagePath == imagePath && !createTilesetPreviewPixels.empty()) {
        return true;
    }

    resetCreateTilesetPreviewImage();
    createTilesetPreviewImagePath = imagePath;

    int width = 0;
    int height = 0;
    int channelCount = 0;
    stbi_uc* pixels = stbi_load(imagePath.c_str(), &width, &height, &channelCount, 4);
    if (pixels == nullptr || width <= 0 || height <= 0) {
        statusMessage = "Failed to load image pixels for eyedropper.";
        if (pixels != nullptr) {
            stbi_image_free(pixels);
        }

        return false;
    }

    const std::size_t pixelCount = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U;
    createTilesetPreviewPixels.assign(pixels, pixels + pixelCount);
    stbi_image_free(pixels);

    createTilesetPreviewImageWidth = width;
    createTilesetPreviewImageHeight = height;
    return true;
}

void LevelEditor::resetCreateTilesetPreviewImage() {
    createTilesetPreviewImagePath.clear();
    createTilesetPreviewImageWidth = 0;
    createTilesetPreviewImageHeight = 0;
    createTilesetPreviewPixels.clear();
}

bool LevelEditor::createTilesetFromTexture(
    TextureAsset& textureAsset,
    const std::string& tilesetName,
    int tileWidth,
    int tileHeight,
    bool useTransparencyColor,
    const float transparencyColor[3]
) {
    const int imageWidth = textureAsset.getWidth();
    const int imageHeight = textureAsset.getHeight();

    if (tilesetName.empty()
        || tileWidth <= 0
        || tileHeight <= 0
        || imageWidth <= 0
        || imageHeight <= 0
        || imageWidth % tileWidth != 0
        || imageHeight % tileHeight != 0) {
        statusMessage = "Failed to create tileset: invalid tileset settings.";
        return false;
    }

    namespace fs = std::filesystem;

    const fs::path outputPath = resolveTilesetOutputPath(newTilesetOutputFile);
    std::error_code directoryError;
    fs::create_directories(outputPath.parent_path(), directoryError);
    if (directoryError) {
        statusMessage = "Failed to create tileset directory: " + directoryError.message();
        return false;
    }

    const fs::path texturePath(textureAsset.getPath());
    std::error_code relativeError;
    fs::path imageSource = fs::relative(texturePath, outputPath.parent_path(), relativeError);
    if (relativeError || imageSource.empty()) {
        imageSource = texturePath.filename();
    }

    const int columns = imageWidth / tileWidth;
    const int rows = imageHeight / tileHeight;
    const int tileCount = columns * rows;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* tileset = document.NewElement("tileset");
    tileset->SetAttribute("version", "1.10");
    tileset->SetAttribute("tiledversion", "1.12.2");
    tileset->SetAttribute("name", tilesetName.c_str());
    tileset->SetAttribute("tilewidth", tileWidth);
    tileset->SetAttribute("tileheight", tileHeight);
    tileset->SetAttribute("tilecount", tileCount);
    tileset->SetAttribute("columns", columns);
    document.InsertEndChild(tileset);

    tinyxml2::XMLElement* image = document.NewElement("image");
    const std::string imageSourceText = imageSource.generic_string();
    image->SetAttribute("source", imageSourceText.c_str());
    if (useTransparencyColor) {
        const std::string transparencyHex = colorToTiledHex(transparencyColor);
        image->SetAttribute("trans", transparencyHex.c_str());
    }

    image->SetAttribute("width", imageWidth);
    image->SetAttribute("height", imageHeight);
    tileset->InsertEndChild(image);

    const tinyxml2::XMLError result = document.SaveFile(outputPath.string().c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to save tileset: " + std::string(document.ErrorStr());
        return false;
    }

    statusMessage = "Created tileset " + outputPath.filename().string() + ".";
    return true;
}

void LevelEditor::resetCreateTilesetForm() {
    selectedCreateTilesetTextureIndex = -1;
    newTilesetTileWidth = 16;
    newTilesetTileHeight = 16;
    newTilesetUseTransparencyColor = true;
    pickingTransparencyColor = false;
    newTilesetTransparencyColor[0] = 0.0f;
    newTilesetTransparencyColor[1] = 0.0f;
    newTilesetTransparencyColor[2] = 1.0f / 255.0f;
    newTilesetName.clear();
    newTilesetOutputFile.clear();
    resetCreateTilesetPreviewImage();
}

bool LevelEditor::saveTilesetAnimations(EditorTileset& tileset) {
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

void LevelEditor::drawViewportGrid() {
    constexpr int MaxGridLinesPerAxis = 512;
    constexpr int MaxGridLabels = 1200;
    constexpr float MinScreenTileSizeForLabels = 32.0f;

    Renderer& renderer = Renderer::getInstance();
    const RenderRect viewport = getViewportForCamera();
    const Camera2D& camera = renderer.getCamera();
    const RenderRect worldViewport = camera.getWorldViewport(viewport);
    const float tileSize = 16.0f * renderer.getRenderScale();
    const float screenTileSize = tileSize * camera.getZoom();

    if (tileSize <= 0.0f
        || screenTileSize <= 0.0f
        || viewport.width <= 0.0f
        || viewport.height <= 0.0f
        || !std::isfinite(worldViewport.x)
        || !std::isfinite(worldViewport.y)
        || !std::isfinite(worldViewport.width)
        || !std::isfinite(worldViewport.height)) {
        return;
    }

    ImDrawList* gridDrawList = ImGui::GetBackgroundDrawList();
    ImDrawList* textDrawList = ImGui::GetForegroundDrawList();

    const ImU32 lineColor = IM_COL32(255, 255, 255, 35);
    const ImU32 textColor = IM_COL32(255, 255, 255, 90);

    const float worldLeft = worldViewport.x;
    const float worldTop = worldViewport.y;
    const float worldRight = worldViewport.x + worldViewport.width;
    const float worldBottom = worldViewport.y + worldViewport.height;

    const int startColumn = static_cast<int>(std::floor(worldLeft / tileSize));
    const int endColumn = static_cast<int>(std::ceil(worldRight / tileSize));
    const int startRow = static_cast<int>(std::floor(worldTop / tileSize));
    const int endRow = static_cast<int>(std::ceil(worldBottom / tileSize));
    const int visibleColumns = std::max(1, endColumn - startColumn);
    const int visibleRows = std::max(1, endRow - startRow);

    if (visibleColumns > MaxGridLinesPerAxis || visibleRows > MaxGridLinesPerAxis) {
        return;
    }

    for (int row = startRow; row <= endRow; ++row) {
        const float worldY = static_cast<float>(row) * tileSize;
        const Vector2F start = camera.worldToScreen(Vector2F(worldLeft, worldY), viewport);
        const Vector2F end = camera.worldToScreen(Vector2F(worldRight, worldY), viewport);
        gridDrawList->AddLine(
            ImVec2(start.x, start.y),
            ImVec2(end.x, end.y),
            lineColor
        );
    }

    for (int column = startColumn; column <= endColumn; ++column) {
        const float worldX = static_cast<float>(column) * tileSize;
        const Vector2F start = camera.worldToScreen(Vector2F(worldX, worldTop), viewport);
        const Vector2F end = camera.worldToScreen(Vector2F(worldX, worldBottom), viewport);
        gridDrawList->AddLine(
            ImVec2(start.x, start.y),
            ImVec2(end.x, end.y),
            lineColor
        );
    }

    const int totalVisibleCells = visibleColumns * visibleRows;
    if (screenTileSize < MinScreenTileSizeForLabels || totalVisibleCells > MaxGridLabels) {
        return;
    }

    for (int row = startRow; row < endRow; ++row) {
        for (int column = startColumn; column < endColumn; ++column) {
            const int id = (row - startRow) * visibleColumns + (column - startColumn);
            const float worldX = static_cast<float>(column) * tileSize;
            const float worldY = static_cast<float>(row) * tileSize;
            const Vector2F screenPosition = camera.worldToScreen(Vector2F(worldX, worldY + tileSize), viewport);

            if (screenPosition.x < viewport.x - screenTileSize || screenPosition.x > viewport.x + viewport.width) {
                continue;
            }

            if (screenPosition.y < viewport.y || screenPosition.y > viewport.y + viewport.height + screenTileSize) {
                continue;
            }

            const ImVec2 textPosition(
                screenPosition.x + 4.0f,
                screenPosition.y - ImGui::GetTextLineHeight() - 3.0f
            );

            textDrawList->AddText(
                textPosition,
                textColor,
                std::to_string(id).c_str()
            );
        }
    }
}
