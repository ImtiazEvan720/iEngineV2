#include "editor/ColliderGizmo.h"

#include "Entity.h"
#include "components/CollisionComponent.h"
#include "components/TransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/ViewportGrid.h"
#include "misc/Level.h"
#include "system/Renderer.h"

namespace {
const char* getEditStatusVerb(RectGizmo::Handle handle) {
    return handle == RectGizmo::Handle::Move
        ? "Moved collider offset for entity "
        : "Updated collider bounds for entity ";
}
}

void ColliderGizmo::cancel() {
    rectGizmo.cancel();
}

RectGizmo::Rect ColliderGizmo::buildColliderRect(
    const TransformComponent& transform,
    const CollisionComponent& collider
) const {
    RectGizmo::Rect rect;
    rect.center = transform.getWorldPosition() + collider.getOffset();
    rect.width = collider.getWidth();
    rect.height = collider.getHeight();
    return rect;
}

void ColliderGizmo::applyColliderRect(
    const TransformComponent& transform,
    CollisionComponent& collider,
    const RectGizmo::Rect& rect
) const {
    const Vector2F transformWorldPosition = transform.getWorldPosition();
    collider.setSize(rect.width, rect.height);
    collider.setOffset(rect.center - transformWorldPosition);
}

void ColliderGizmo::draw(
    const EntityInspectorPanel& entityInspector,
    const EditorCamera& camera
) const {
    const int selectedEntityId = entityInspector.getSelectedEntityId();
    if (selectedEntityId < 0) {
        return;
    }

    Entity* entity = Level::getCurrentLevel().findEntityById(selectedEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    const RenderRect viewport = camera.getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return;
    }

    rectGizmo.drawHandles(
        this->buildColliderRect(*transform, *collider),
        Renderer::getInstance().getCamera(),
        viewport
    );
}

bool ColliderGizmo::handleInteraction(
    EntityInspectorPanel& entityInspector,
    const ViewportGrid& viewportGrid,
    const Vector2F& worldMousePosition,
    const RenderRect& viewport,
    int& draggingEntityId,
    std::string& statusMessage
) {

    if (!viewportGrid.shouldShowColliders()) {
        cancel();
        return false;
    }

    Entity* entity = Level::getCurrentLevel().findEntityById(entityInspector.getSelectedEntityId());
    if (entity == nullptr) {
        return false;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return false;
    }

    RectGizmo::Options options;
    options.allowMove = true;
    options.allowResize = true;

    const RectGizmo::EditResult result = rectGizmo.handleInteraction(
        entity->getId(),
        this->buildColliderRect(*transform, *collider),
        Renderer::getInstance().getCamera(),
        viewport,
        worldMousePosition,
        viewportGrid.shouldSnapToGrid(),
        Vector2F(viewportGrid.getGridSize(), viewportGrid.getGridSize()),
        options
    );

    if (!result.active && !result.changed && !result.finished) {
        return false;
    }

    draggingEntityId = -1;

    if (result.changed) {
        this->applyColliderRect(*transform, *collider, result.rect);
    }

    if (result.changed || result.finished) {
        entityInspector.syncEditStateFromEntity(*entity, true);
    }

    if (result.finished) {
        statusMessage = std::string(getEditStatusVerb(result.handle))
            + std::to_string(entity->getId()) + ".";
    } else if (result.active) {
        statusMessage = result.handle == RectGizmo::Handle::Move
            ? "Moving collider."
            : "Editing collider bounds.";
    }

    return true;
}
