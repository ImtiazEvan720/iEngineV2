#ifndef IENGINEV2_LEVELEDITORVIEWPORT_H
#define IENGINEV2_LEVELEDITORVIEWPORT_H

#include "editor/ColliderGizmo.h"
#include "editor/TransformGizmo.h"

#include <string>

class EditorCamera;
class Entity;
class EntityInspectorPanel;
class PrefabPanel;
class SpritePalettePanel;
class ViewportGrid;
class ColliderGizmo;
class TransformGizmo;

class LevelEditorViewport {
public:
    bool draw(
        bool enabled,
        EditorCamera& camera,
        ViewportGrid& viewportGrid,
        EntityInspectorPanel& entityInspector,
        SpritePalettePanel& spritePalette,
        PrefabPanel& prefabPanel,
        float toolbarHeight,
        std::string& statusMessage
    );

private:
    bool handleEntityInteraction(
        bool enabled,
        EditorCamera& camera,
        const ViewportGrid& viewportGrid,
        EntityInspectorPanel& entityInspector,
        std::string& statusMessage
    );
    Entity* findEntityAt(float x, float y, const EditorCamera& camera, const ViewportGrid& viewportGrid);
    Entity* findEntityById(int id) const;
    bool entityContainsPoint(
        Entity& entity,
        float x,
        float y,
        const EditorCamera& camera,
        const ViewportGrid& viewportGrid
    ) const;

    int draggingEntityId = -1;
    float dragOffset[2] = {0.0f, 0.0f};
    ColliderGizmo colliderGizmo;
    TransformGizmo transformGizmo;
};

#endif
