#pragma once

#include "editor/EditorCamera.h"
#include "editor/RectGizmo.h"

#include <string>

class EntityInspectorPanel;
class EditorCamera;
class RectTransformComponent;
class ViewportGrid;

class RectTransformGizmo {
public:
    void cancel();
    void draw(
        const EntityInspectorPanel& entityInspector,
        const EditorCamera& camera
    ) const;
    bool handleInteraction(
        EntityInspectorPanel& entityInspector,
        const ViewportGrid& viewportGrid,
        const Vector2F& screenMousePosition,
        const RenderRect& viewport,
        int& draggingEntityId,
        std::string& statusMessage
    );

private:
    void calculateRect(RectGizmo::Rect& rect, const RectTransformComponent& rectTransform) const;
    void applyRect(const RectGizmo::Rect& rect, RectTransformComponent& rectTransform);

    RectGizmo::Rect rect = RectGizmo::Rect();
    RectGizmo rectGizmo;
};
