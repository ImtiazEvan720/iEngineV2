#include "editor/LevelEditor.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
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

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <iostream>

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

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
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

    const int column = tileId % tileset.columns;
    const int row = tileId / tileset.columns;
    const float sourceX = static_cast<float>(column * tileset.tileWidth);
    const float sourceY = static_cast<float>(row * tileset.tileHeight);
    const float renderScale = Renderer::getInstance().getRenderScale();

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

    Entity& entity = Level::getCurrentLevel().createEntity();
    entity.setName("EditorSprite_" + std::to_string(createdSpriteCount));
    entity.setTag("EditorSprite");
    entity.addComponent<TransformComponent>(Vector2F(x, y), 0.0f);
    entity.addComponent<SpriteComponent>(sprite);

    ++createdSpriteCount;
    selectedTileId = tileId;
    statusMessage = "Created sprite entity from tile id " + std::to_string(tileId) + ".";
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
