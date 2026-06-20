#include "editor/LevelEditorViewport.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/PrefabPanel.h"
#include "editor/SpritePalettePanel.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/Renderer.h"
#include "system/EngineState.h"

#include "imgui.h"

bool LevelEditorViewport::draw(
    bool enabled,
    EditorCamera& camera,
    ViewportGrid& viewportGrid,
    EntityInspectorPanel& entityInspector,
    SpritePalettePanel& spritePalette,
    PrefabPanel& prefabPanel,
    float toolbarHeight,
    std::string& statusMessage
) {
    Renderer& renderer = Renderer::getInstance();
    EngineState& engine = EngineState::getInstance();
    const bool activeDragDrop = ImGui::GetDragDropPayload() != nullptr;
    const bool transformHandlesVisible = enabled;
    const bool isGamePlaying = engine.isPlaying();
    const bool activeEditorViewport =
        renderer.shouldRenderEditorViewport()
        || activeDragDrop
        || transformHandlesVisible
        || (enabled && (viewportGrid.shouldShowGrid() || viewportGrid.shouldShowColliders()));

    if (activeEditorViewport && viewportGrid.shouldShowGrid()) {
        viewportGrid.draw(camera.getViewport());
    }

    if (activeEditorViewport && transformHandlesVisible && !isGamePlaying) {
        transformGizmo.draw(Level::getCurrentLevel(), entityInspector, camera);
    }

    if (activeEditorViewport && viewportGrid.shouldShowColliders()) {
        colliderGizmo.draw(entityInspector, camera);
    }

    const bool moveToolRequested = handleEntityInteraction(
        enabled,
        camera,
        viewportGrid,
        entityInspector,
        statusMessage
    );

    if (activeEditorViewport) {
        spritePalette.drawLevelDropTarget(
            viewportGrid,
            camera.getViewport(),
            toolbarHeight,
            statusMessage
        );
        prefabPanel.drawLevelDropTarget(
            viewportGrid,
            camera.getViewport(),
            toolbarHeight,
            statusMessage
        );
    }

    return moveToolRequested;
}

bool LevelEditorViewport::handleEntityInteraction(
    bool enabled,
    EditorCamera& camera,
    const ViewportGrid& viewportGrid,
    EntityInspectorPanel& entityInspector,
    std::string& statusMessage
) {
    if (!enabled) {
        draggingEntityId = -1;
        colliderGizmo.cancel();
        return false;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return false;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (camera.isPanActive()) {
        draggingEntityId = -1;
        colliderGizmo.cancel();
        return false;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const RenderRect viewport = camera.getViewport();
    Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const Vector2F worldMousePosition = sceneCamera.screenToWorld(
        Vector2F(mousePosition.x, mousePosition.y),
        viewport
    );

    if (colliderGizmo.handleInteraction(
            entityInspector,
            viewportGrid,
            camera,
            worldMousePosition,
            viewport,
            draggingEntityId,
            statusMessage)) {
        return true;
    }

    bool moveToolRequested = false;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        Entity* entity = findEntityAt(worldMousePosition.x, worldMousePosition.y, camera, viewportGrid);
        if (entity != nullptr) {
            entityInspector.selectEntity(*entity, true);
            draggingEntityId = entity->getId();
            moveToolRequested = true;

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
            return moveToolRequested;
        }

        TransformComponent* transform = entity->getComponent<TransformComponent>();
        if (transform == nullptr) {
            draggingEntityId = -1;
            return moveToolRequested;
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

    return moveToolRequested;
}

Entity* LevelEditorViewport::findEntityAt(
    float x,
    float y,
    const EditorCamera& camera,
    const ViewportGrid& viewportGrid
) {
    auto& entities = Level::getCurrentLevel().getEntities();

    for (auto iterator = entities.rbegin(); iterator != entities.rend(); ++iterator) {
        if (iterator->isDestroyed()) {
            continue;
        }

        if (entityContainsPoint(*iterator, x, y, camera, viewportGrid)) {
            return &*iterator;
        }
    }

    return nullptr;
}

Entity* LevelEditorViewport::findEntityById(int id) const {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (entity.getId() == id && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}

bool LevelEditorViewport::entityContainsPoint(
    Entity& entity,
    float x,
    float y,
    const EditorCamera& camera,
    const ViewportGrid& viewportGrid
) const {
    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return false;
    }

    if (transformGizmo.contains(entity, x, y, camera)) {
        return true;
    }

    if (viewportGrid.shouldShowColliders()) {
        if (CollisionComponent* collider = entity.getComponent<CollisionComponent>()) {
            Vector2F worldPosition = transform->getWorldPosition();
            const Vector2F& offset = collider->getOffset();
            worldPosition.x += offset.x;
            worldPosition.y += offset.y;
            const float halfWidth = collider->getWidth() * 0.5f;
            const float halfHeight = collider->getHeight() * 0.5f;
            const float left = worldPosition.x - halfWidth;
            const float right = worldPosition.x + halfWidth;
            const float top = worldPosition.y - halfHeight;
            const float bottom = worldPosition.y + halfHeight;

            if (x >= left && x <= right && y >= top && y <= bottom) {
                return true;
            }
        }
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
