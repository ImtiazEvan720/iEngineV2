#include "editor/RectTransformGizmo.h"

#include "components/RectTransformComponent.h"
#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/RectGizmo.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Level.h"

#include <string>

void RectTransformGizmo::cancel() {
    rectGizmo.cancel();
}

void RectTransformGizmo::draw(
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

    RectTransformComponent* rectTransform = entity->getComponent<RectTransformComponent>();
    if (rectTransform == nullptr) {
        return;
    }

    const RenderRect viewport = camera.getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return;
    }

    RectGizmo::Rect rect;
    this->calculateRect(rect, *rectTransform);
    rectGizmo.drawHandlesScreenSpace(
        rect,
        viewport
    );
}

bool RectTransformGizmo::handleInteraction(
    EntityInspectorPanel& entityInspector,
    const ViewportGrid& viewportGrid,
    const Vector2F& screenMousePosition,
    const RenderRect& viewport,
    int& draggingEntityId,
    std::string& statusMessage
) {
    Entity* entity = Level::getCurrentLevel().findEntityById(entityInspector.getSelectedEntityId());
    if (entity == nullptr) {
        return false;
    }

    RectTransformComponent* rectTransform = entity->getComponent<RectTransformComponent>();
    if (rectTransform == nullptr) {
        return false;
    }

    RectGizmo::Rect currentRect;
    calculateRect(currentRect, *rectTransform);

    RectGizmo::Options options;
    options.allowMove = false;
    options.allowResize = true;

    const RectGizmo::EditResult result = rectGizmo.handleInteractionScreenSpace(
        entity->getId(),
        currentRect,
        viewport,
        screenMousePosition,
        viewportGrid.shouldSnapToGrid(),
        Vector2F(viewportGrid.getGridSize(), viewportGrid.getGridSize()),
        options
    );

    if (!result.active && !result.changed && !result.finished) {
        return false;
    }

    draggingEntityId = -1;

    if (result.changed) {
        applyRect(result.rect, *rectTransform);
    }

    if (result.changed || result.finished) {
        entityInspector.syncEditStateFromEntity(*entity, true);
    }

    if (result.finished) {
        statusMessage = "Updated RectTransformComponent bounds for entity "
            + std::to_string(entity->getId()) + ".";
    } else if (result.active) {
        statusMessage = "Editing RectTransformComponent bounds.";
    }

    return true;
}

void RectTransformGizmo::calculateRect(
    RectGizmo::Rect& rect,
    const RectTransformComponent& rectTransform
) const {
    rect.center = rectTransform.getAnchoredPosition();
    rect.width = rectTransform.getSize().x;
    rect.height = rectTransform.getSize().y;
}

void RectTransformGizmo::applyRect(
    const RectGizmo::Rect& rect,
    RectTransformComponent& rectTransform
) {
    rectTransform.setAnchoredPosition(rect.center);
    rectTransform.setSize({rect.width, rect.height});
}
