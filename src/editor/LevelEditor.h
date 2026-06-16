#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include <string>
#include <vector>

class InputSystem;
class Entity;
class AnimationComponent;
class CollisionComponent;
class SpriteComponent;
class TextureAsset;
class TransformComponent;
class Vector2F;

class LevelEditor {
public:
    enum class Tool {
        Select,
        Move
    };

    void draw(const InputSystem& inputSystem, float windowWidth);

    void setEnabled(bool value);
    bool isEnabled() const;
    void toggleEnabled();

    Tool getCurrentTool() const;
    float getToolbarHeight() const;
    bool shouldShowGrid() const;
    bool shouldShowColliders() const;

private:
    struct EditorTileset {
        std::string name;
        std::string path;
        std::string imagePath;
        std::string imageFilename;
        int tileWidth = 0;
        int tileHeight = 0;
        int columns = 0;
        int tileCount = 0;
        int imageWidth = 0;
        int imageHeight = 0;
        TextureAsset* textureAsset = nullptr;
    };

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

    void drawLevelOutlineTab(const InputSystem& inputSystem);
    void drawEntitiesTab();
    void drawSpritesTab();
    void drawFileExplorerTab();
    void refreshTilesets();
    void drawTilesetSelector();
    void drawSelectedTilesetGrid();
    void drawLevelDropTarget();
    void drawEntityTreeNode(Entity& entity);
    void drawEntityComponents(Entity& entity);
    void drawEntityIdentityFields(Entity& entity);
    void drawTransformComponentFields(TransformComponent& transform);
    void drawSpriteComponentFields(SpriteComponent& spriteComponent);
    void drawAnimationComponentFields(AnimationComponent& animationComponent);
    void drawCollisionComponentFields(CollisionComponent& collisionComponent);
    void handleViewportEntityInteraction();
    Entity* findEntityAt(float x, float y);
    Entity* findEntityById(int id);
    bool entityContainsPoint(Entity& entity, float x, float y) const;
    void syncEditStateFromEntity(Entity& entity, bool force = false);
    void createSpriteEntityFromTile(int tilesetIndex, int tileId, float x, float y);

    bool enabled = true;
    bool showGrid = true;
    bool showColliders = false;
    bool tilesetsScanned = false;
    float tilePreviewScale = 3.0f;
    int selectedTilesetIndex = -1;
    int selectedTileId = -1;
    int selectedEntityId = -1;
    int draggingEntityId = -1;
    int createdSpriteCount = 0;
    Tool currentTool = Tool::Select;
    float dragOffset[2] = {0.0f, 0.0f};
    std::string statusMessage;
    EntityEditState entityEditState;
    std::vector<EditorTileset> tilesets;
};

#endif
