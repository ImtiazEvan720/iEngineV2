#pragma once

#include "editor/ScriptPropertyInspector.h"
#include "editor/SpritePickerWidget.h"

#include <filesystem>
#include <string>
#include <vector>

class AnimationComponent;
class CollisionComponent;
class Entity;
class Level;
class PlayerCameraComponent;
class ScriptComponent;
class SpriteComponent;
class TransformComponent;

class EntityInspectorPanel {
public:
    bool draw(std::string& statusMessage);

    int getSelectedEntityId() const;
    void selectEntity(Entity& entity, bool forceSync = false);
    void clearSelection();
    void requestDeleteSelected(std::string& statusMessage);
    void processPendingDelete(Level& level, std::string& statusMessage);
    void syncEditStateFromEntity(Entity& entity, bool force = false);

private:
    struct EntityEditState {
        int entityId = -1;
        std::string name;
        std::string tag;
        bool enabled = true;
        float position[2] = {0.0f, 0.0f};
        float rotation = 0.0f;
        float spriteSource[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float spriteSize[2] = {0.0f, 0.0f};
        float spriteOrigin[2] = {0.0f, 0.0f};
        float animationFrameDuration = 0.0f;
        bool animationPlaying = false;
        bool animationLooping = true;
        float collisionOffset[2] = {0.0f, 0.0f};
        float collisionSize[2] = {0.0f, 0.0f};
        int collisionBodyType = 0;
        bool collisionSensor = false;
        std::string collisionName;
        float cameraZoom = 1.0f;
        float cameraViewportSize[2] = {1280.0f, 720.0f};
        float cameraOffset[2] = {0.0f, 0.0f};
        bool cameraClampToBounds = false;
        float cameraBoundsMin[2] = {0.0f, 0.0f};
        float cameraBoundsMax[2] = {1280.0f, 720.0f};
    };

    void drawEntityTreeNode(Entity& entity, std::string& statusMessage);
    void drawEntityComponents(Entity& entity, std::string& statusMessage);
    void drawAddComponentCombo(Entity& entity, std::string& statusMessage);
    bool drawRemoveComponentButton(Entity& entity, const char* componentName, std::string& statusMessage);
    void drawEntityIdentityFields(Entity& entity, std::string& statusMessage);
    bool drawPrefabActions(Level& level, std::string& statusMessage);
    Entity* findSelectedEntity(Level& level) const;
    void drawTransformComponentFields(TransformComponent& transform, std::string& statusMessage);
    void drawSpriteComponentFields(SpriteComponent& spriteComponent, std::string& statusMessage);
    void drawAnimationComponentFields(AnimationComponent& animationComponent, std::string& statusMessage);
    void refreshAnimationFiles();
    bool loadAnimationFileIntoComponent(
        const std::filesystem::path& animationPath,
        AnimationComponent& animationComponent,
        std::string& statusMessage
    );
    void drawCollisionComponentFields(CollisionComponent& collisionComponent, std::string& statusMessage);
    void drawPlayerCameraComponentFields(PlayerCameraComponent& cameraComponent, std::string& statusMessage);
    void drawScriptComponentFields(ScriptComponent& scriptComponent, std::string& statusMessage);

    int selectedEntityId = -1;
    int pendingDeleteEntityId = -1;
    int selectedAddComponentIndex = 0;
    int selectedScriptIndex = 0;
    int selectedAnimationFileIndex = -1;
    bool animationFilesScanned = false;
    std::string prefabName;
    std::vector<std::string> animationFileLabels;
    std::vector<std::filesystem::path> animationFilePaths;
    SpritePickerWidget spritePicker;
    SpritePickerWidget animationPicker;
    ScriptPropertyInspector scriptPropertyInspector;
    EntityEditState entityEditState;
};
