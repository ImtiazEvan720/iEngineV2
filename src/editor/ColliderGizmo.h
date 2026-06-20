#ifndef IENGINEV2_COLLIDERGIZMO_H
#define IENGINEV2_COLLIDERGIZMO_H

#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

#include <string>

class EditorCamera;
class Entity;
class EntityInspectorPanel;
class ViewportGrid;

class ColliderGizmo {
public:
    void cancel();
    void draw(const EntityInspectorPanel& entityInspector, const EditorCamera& camera) const;
    bool handleInteraction(
        EntityInspectorPanel& entityInspector,
        const ViewportGrid& viewportGrid,
        const EditorCamera& camera,
        const Vector2F& worldMousePosition,
        const RenderRect& viewport,
        int& draggingEntityId,
        std::string& statusMessage
    );

private:
    enum class EditMode {
        None,
        Move,
        ResizeLeft,
        ResizeRight,
        ResizeTop,
        ResizeBottom,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight
    };

    EditMode hitTestSelectedHandle(
        const EntityInspectorPanel& entityInspector,
        const Vector2F& screenMousePosition,
        const RenderRect& viewport
    ) const;
    void moveSelectedCollider(
        EntityInspectorPanel& entityInspector,
        const ViewportGrid& viewportGrid,
        const Vector2F& worldMousePosition
    );
    void resizeSelectedCollider(
        EntityInspectorPanel& entityInspector,
        const ViewportGrid& viewportGrid,
        const Vector2F& worldMousePosition
    );
    Entity* findEntityById(int id) const;

    int editingEntityId = -1;
    EditMode editMode = EditMode::None;
    float dragOffset[2] = {0.0f, 0.0f};
};

#endif
