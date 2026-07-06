#include "editor/RectTransformGizmo.h"
#include "components/RectTransformComponent.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/RectGizmo.h"
#include "editor/EditorCamera.h"
#include "misc/Level.h"
#include "math/Vector2F.h"
#include "system/Renderer.h"

void RectTransformGizmo::draw(const EntityInspectorPanel &entityInspector,
                              const EditorCamera &camera) const 
{
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
    rectGizmo.drawHandles(
        rect,
        Renderer::getInstance().getCamera(),
        viewport
    );

}
void RectTransformGizmo::calculateRect(
    RectGizmo::Rect &rect, const RectTransformComponent &rectTransform) const {
  rect.center = rectTransform.getAnchoredPosition();
  rect.width = rectTransform.getSize().x;
  rect.height = rectTransform.getSize().y;
}

void RectTransformGizmo::applyRect(const RectGizmo::Rect &rect,
                                   RectTransformComponent &rectTransform) {
  rectTransform.setAnchoredPosition(rect.center);
  rectTransform.setSize({rect.width, rect.height});
}