#ifndef IENGINEV2_ENTITYINSPECTORPANEL_H
#define IENGINEV2_ENTITYINSPECTORPANEL_H

#include <string>

class AnimationComponent;
class CollisionComponent;
class Entity;
class Level;
class ScriptComponent;
class SpriteComponent;
class TransformComponent;

class EntityInspectorPanel {
public:
    bool draw(std::string& statusMessage);

    int getSelectedEntityId() const;
    void selectEntity(Entity& entity, bool forceSync = false);
    void clearSelection();
    void syncEditStateFromEntity(Entity& entity, bool force = false);

private:
    struct EntityEditState {
        int entityId = -1;
        std::string name;
        std::string tag;
        float position[2] = {0.0f, 0.0f};
        float rotation = 0.0f;
        float spriteSource[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float spriteSize[2] = {0.0f, 0.0f};
        float spriteOrigin[2] = {0.0f, 0.0f};
        float animationFrameDuration = 0.0f;
        bool animationPlaying = false;
        float collisionSize[2] = {0.0f, 0.0f};
        int collisionBodyType = 0;
        bool collisionSensor = false;
        std::string collisionName;
    };

    void drawEntityTreeNode(Entity& entity, std::string& statusMessage);
    void drawEntityComponents(Entity& entity, std::string& statusMessage);
    void drawEntityIdentityFields(Entity& entity, std::string& statusMessage);
    bool drawPrefabActions(Level& level, std::string& statusMessage);
    Entity* findSelectedEntity(Level& level) const;
    void drawTransformComponentFields(TransformComponent& transform, std::string& statusMessage);
    void drawSpriteComponentFields(SpriteComponent& spriteComponent, std::string& statusMessage);
    void drawAnimationComponentFields(AnimationComponent& animationComponent, std::string& statusMessage);
    void drawCollisionComponentFields(CollisionComponent& collisionComponent, std::string& statusMessage);
    void drawScriptComponentFields(ScriptComponent& scriptComponent);

    int selectedEntityId = -1;
    std::string prefabName;
    EntityEditState entityEditState;
};

#endif
