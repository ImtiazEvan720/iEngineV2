#pragma once

#include "editor/RectGizmo.h"
#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

#include <string>

class EditorCamera;
class EntityInspectorPanel;
class ViewportGrid;
class CollisionComponent;
class TransformComponent;

class ColliderGizmo {
public:
    void cancel();
    void draw(const EntityInspectorPanel& entityInspector, const EditorCamera& camera) const;
    bool handleInteraction(
        EntityInspectorPanel& entityInspector,
        const ViewportGrid& viewportGrid,
        const Vector2F& worldMousePosition,
        const RenderRect& viewport,
        int& draggingEntityId,
        std::string& statusMessage
    );

private:
    RectGizmo::Rect buildColliderRect(
        const TransformComponent& transform,
        const CollisionComponent& collider
    ) const;
    void applyColliderRect(
        const TransformComponent& transform,
        CollisionComponent& collider,
        const RectGizmo::Rect& rect
    ) const;

    RectGizmo rectGizmo;
};
