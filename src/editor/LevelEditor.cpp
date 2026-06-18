#include "editor/LevelEditor.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/InputSystem.h"
#include "system/Renderer.h"

#include "imgui.h"

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
            viewportGrid.drawMenuItems();
            ImGui::EndMenu();
        }

        ImGui::EndMainMenuBar();
    }

    if (tilesetCreator.draw(statusMessage)) {
        spritePalette.refreshTilesets();
    }

    if (viewportGrid.shouldShowSideMenu()) {
        const bool levelEditorOpen = ImGui::Begin("Level Editor");

        if (levelEditorOpen && ImGui::BeginTabBar("##LevelEditorTabs")) {
            if (ImGui::BeginTabItem("Level_Outline")) {
                drawLevelOutlineTab(inputSystem);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Entities")) {
                if (entityInspector.draw(statusMessage)) {
                    prefabPanel.refreshPrefabs();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Sprites")) {
                spritePalette.draw(statusMessage);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Prefabs")) {
                prefabPanel.draw(statusMessage);
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

    camera.beginFrame();
    camera.handleZoom(enabled, statusMessage);
    camera.handlePan(enabled);

    if (viewportGrid.shouldShowGrid()) {
        viewportGrid.draw(camera.getViewport());
    }

    handleViewportEntityInteraction();
    spritePalette.drawLevelDropTarget(
        viewportGrid,
        camera.getViewport(),
        getToolbarHeight(),
        statusMessage
    );
    prefabPanel.drawLevelDropTarget(
        viewportGrid,
        camera.getViewport(),
        getToolbarHeight(),
        statusMessage
    );
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
    return viewportGrid.shouldShowGrid();
}

bool LevelEditor::shouldShowColliders() const {
    return viewportGrid.shouldShowColliders();
}

bool LevelEditor::shouldSnapToGrid() const {
    return viewportGrid.shouldSnapToGrid();
}

void LevelEditor::drawLevelOutlineTab(const InputSystem& inputSystem) {
    ImGui::Text("Input: %s", inputSystem.getLastInputText().c_str());
    ImGui::Text("Editor: %s", enabled ? "Enabled" : "Disabled");
    ImGui::Text("Tool: %s", currentTool == Tool::Select ? "Select" : "Move");
}

void LevelEditor::drawFileExplorerTab() {
    ImGui::Text("File explorer goes here.");
}

void LevelEditor::drawAssetsMenu() {
    if (!ImGui::BeginMenu("Assets")) {
        return;
    }

    if (ImGui::MenuItem("Create Tileset")) {
        tilesetCreator.open();
    }

    if (ImGui::MenuItem("Refresh Tilesets")) {
        spritePalette.refreshTilesets();
        statusMessage = "Refreshed tilesets.";
    }

    if (ImGui::MenuItem("Refresh Prefabs")) {
        prefabPanel.refreshPrefabs();
        statusMessage = "Refreshed prefabs.";
    }

    ImGui::EndMenu();
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
    if (camera.isPanActive()) {
        draggingEntityId = -1;
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const RenderRect viewport = camera.getViewport();
    Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const Vector2F worldMousePosition = sceneCamera.screenToWorld(
        Vector2F(mousePosition.x, mousePosition.y),
        viewport
    );

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        Entity* entity = findEntityAt(worldMousePosition.x, worldMousePosition.y);
        if (entity != nullptr) {
            entityInspector.selectEntity(*entity, true);
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

            statusMessage = "Selected entity " + std::to_string(entity->getId()) + ".";
        } else {
            entityInspector.clearSelection();
            draggingEntityId = -1;
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
        if (viewportGrid.shouldSnapToGrid()) {
            newPosition = viewportGrid.snapPosition(newPosition);
        }

        transform->setPosition(newPosition);
        entityInspector.syncEditStateFromEntity(*entity, true);
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        Entity* entity = findEntityById(draggingEntityId);
        if (entity != nullptr) {
            entityInspector.syncEditStateFromEntity(*entity, true);
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
