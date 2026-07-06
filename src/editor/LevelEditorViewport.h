#pragma once

#include "editor/ColliderGizmo.h"
#include "editor/RectTransformGizmo.h"
#include "editor/TransformGizmo.h"
#include "math/Vector2F.h"

#include <string>

class EditorCamera;
class Entity;
class EntityInspectorPanel;
class PrefabPanel;
class SpritePalettePanel;
class ViewportGrid;
class ColliderGizmo;
class TransformGizmo;
class RectTransformGizmo;

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
    Entity* findEntityAt(
        const Vector2F& screenMousePosition,
        const EditorCamera& camera,
        const ViewportGrid& viewportGrid
    ) const;

    bool entityContainsPoint(
        Entity& entity,
        const Vector2F& worldSpacePosition,
        const EditorCamera& camera,
        const ViewportGrid& viewportGrid
    ) const;

    bool uiEntityContainsPoint(
        const Entity& entity,
        const Vector2F& screenSpacePosition
    ) const;

    Entity* findWorldspaceEntityAt(
        const Vector2F& worldSpace,
        const EditorCamera& camera,
        const ViewportGrid& viewportGrid
    ) const;

    Entity* findUiEntityAt(
        const Vector2F& screenSpace
    ) const;


    int draggingEntityId = -1;
    float dragOffset[2] = {0.0f, 0.0f};
    ColliderGizmo colliderGizmo;
    TransformGizmo transformGizmo;
    RectTransformGizmo rectTransformGizmo;
};
