#ifndef IENGINEV2_TRANSFORMGIZMO_H
#define IENGINEV2_TRANSFORMGIZMO_H

class EditorCamera;
class Entity;
class EntityInspectorPanel;
class Level;

class TransformGizmo {
public:
    void draw(Level& level, const EntityInspectorPanel& entityInspector, const EditorCamera& camera) const;
    bool contains(Entity& entity, float worldX, float worldY, const EditorCamera& camera) const;
};

#endif
