#include "editor/LevelEditorViewport.h"

#include "Application.h"
#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/RectTransformComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/PrefabPanel.h"
#include "editor/SpritePalettePanel.h"
#include "editor/ViewportGrid.h"
#include "math/Math2D.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/Renderer.h"
#include "system/EngineState.h"

#include "imgui.h"

#include <cstddef>

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

    if (activeEditorViewport && viewportGrid.shouldShowColliders() && !isGamePlaying) {
        colliderGizmo.draw(entityInspector, camera);
    }

    if(activeEditorViewport && transformHandlesVisible && !isGamePlaying)
    {
        rectTransformGizmo.draw(entityInspector,camera);
    }

    bool moveToolRequested = false;

    if(!isGamePlaying)
    {
        moveToolRequested = handleEntityInteraction(
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
        rectTransformGizmo.cancel();
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
        rectTransformGizmo.cancel();
        return false;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const Vector2F screenMousePosition(mousePosition.x, mousePosition.y);
    const RenderRect viewport = camera.getViewport();
    Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const Vector2F worldMousePosition = sceneCamera.screenToWorld(
        screenMousePosition,
        viewport
    );

    if (rectTransformGizmo.handleInteraction(
            entityInspector,
            viewportGrid,
            screenMousePosition,
            viewport,
            draggingEntityId,
            statusMessage)) {
        return true;
    }

    if (colliderGizmo.handleInteraction(
            entityInspector,
            viewportGrid,
            worldMousePosition,
            viewport,
            draggingEntityId,
            statusMessage)) {
        return true;
    }

    bool moveToolRequested = false;

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        Entity* entity = findEntityAt(screenMousePosition, camera, viewportGrid);
        if (entity != nullptr) {
            entityInspector.selectEntity(*entity, true);
            draggingEntityId = entity->getId();
            moveToolRequested = true;

            if (RectTransformComponent* rectTransform = entity->getComponent<RectTransformComponent>()) {
                const Vector2F position = rectTransform->getWorldPosition();
                dragOffset[0] = screenMousePosition.x - position.x;
                dragOffset[1] = screenMousePosition.y - position.y;
            } else if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
                const Vector2F position = transform->getWorldPosition();
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
        Entity* entity = Level::getCurrentLevel().findEntityById(draggingEntityId);
        if (entity == nullptr) {
            draggingEntityId = -1;
            return moveToolRequested;
        }

        RectTransformComponent* rectTransform = entity->getComponent<RectTransformComponent>();
        TransformComponent* transform = entity->getComponent<TransformComponent>();

        if (transform == nullptr && rectTransform == nullptr) {
            draggingEntityId = -1;
            return moveToolRequested;
        }

        Vector2F newPosition = Vector2F::zero();

        if (rectTransform != nullptr) {
            newPosition = Vector2F(
                screenMousePosition.x - dragOffset[0],
                screenMousePosition.y - dragOffset[1]
            );

            if (viewportGrid.shouldSnapToGrid()) {
                newPosition = viewportGrid.snapPosition(newPosition);
            }

            rectTransform->setWorldPosition(newPosition);
        } else if (transform != nullptr) {
            newPosition = Vector2F(
                worldMousePosition.x - dragOffset[0],
                worldMousePosition.y - dragOffset[1]
            );
            if (viewportGrid.shouldSnapToGrid()) {
                newPosition = viewportGrid.snapPosition(newPosition);
            }

            transform->setWorldPosition(newPosition);
        }

        entityInspector.syncEditStateFromEntity(*entity, true);
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        Entity* entity = Level::getCurrentLevel().findEntityById(draggingEntityId);
        if (entity != nullptr) {
            entityInspector.syncEditStateFromEntity(*entity, true);
            statusMessage = "Moved entity " + std::to_string(entity->getId()) + ".";
        }

        draggingEntityId = -1;
    }

    return moveToolRequested;
}

Entity* LevelEditorViewport::findEntityAt(
    const Vector2F& screenMousePosition,
    const EditorCamera& camera,
    const ViewportGrid& viewportGrid
) const {
    auto* uiEntity = findUiEntityAt(screenMousePosition);

    if (uiEntity != nullptr) {
        return uiEntity;
    }

    const RenderRect viewport = camera.getViewport();
    const Vector2F worldMousePosition = Renderer::getInstance().getCamera().screenToWorld(
        screenMousePosition,
        viewport
    );

    return findWorldspaceEntityAt(worldMousePosition, camera, viewportGrid);
}

Entity* LevelEditorViewport::findWorldspaceEntityAt(
    const Vector2F& worldSpace,
    const EditorCamera& camera,
    const ViewportGrid& viewportGrid
) const {
    auto& entities = Level::getCurrentLevel().getEntities();

    for (auto iterator = entities.rbegin(); iterator != entities.rend(); ++iterator) {
        if (iterator->isDestroyed()) {
            continue;
        }

        if (entityContainsPoint(*iterator, worldSpace, camera, viewportGrid)) {
            return &*iterator;
        }
    }

    return nullptr;
}


Entity* LevelEditorViewport::findUiEntityAt(const Vector2F& screenSpace) const {
    auto& entities = Level::getCurrentLevel().getEntities();

    for (auto iterator = entities.begin(); iterator != entities.end(); ++iterator) {
        Entity& entity = *iterator;

        if (entity.isDestroyed() || !entity.isEnabled()) {
            continue;
        }

        for (Entity* child : entity.getChildren()) {
            if (child != nullptr && !child->isDestroyed() && child->isEnabled()) {
                if (uiEntityContainsPoint(*child, screenSpace)) {
                    return child;
                }
            }
        }

        if (uiEntityContainsPoint(entity, screenSpace)) {
            return &entity;
        }

    }

    return nullptr;
}

bool LevelEditorViewport::entityContainsPoint(
    Entity& entity,
    const Vector2F& worldSpacePosition,
    const EditorCamera& camera,
    const ViewportGrid& viewportGrid
) const {
    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return false;
    }

    if (transformGizmo.contains(entity, worldSpacePosition.x, worldSpacePosition.y, camera)) {
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

            if (worldSpacePosition.x >= left
                && worldSpacePosition.x <= right
                && worldSpacePosition.y >= top
                && worldSpacePosition.y <= bottom) {
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

    return worldSpacePosition.x >= left
        && worldSpacePosition.x <= right
        && worldSpacePosition.y >= top
        && worldSpacePosition.y <= bottom;
}

bool LevelEditorViewport::uiEntityContainsPoint(
    const Entity& entity,
    const Vector2F& screenSpacePosition
) const {
    auto* rect = entity.getComponent<RectTransformComponent>();
    if (rect == nullptr) {
        return false;
    }

    const Vector2F center = rect->getWorldPosition();
    const Vector2F size =
        rect->getSize() * Renderer::getInstance().getUiScale(entity);
    const Vector2F& pivot = rect->getPivot();
    const Vector2F localPosition = Math2D::inverseRotate(
        screenSpacePosition - center,
        rect->getWorldRotation()
    );

    const float left = -(size.x * pivot.x);
    const float top = -(size.y * pivot.y);
    const float right = left + size.x;
    const float bottom = top + size.y;

    return localPosition.x >= left
        && localPosition.x <= right
        && localPosition.y >= top
        && localPosition.y <= bottom;
}
