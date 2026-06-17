#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include "system/IRenderBackend.h"

#include <string>
#include <unordered_map>
#include <vector>

class InputSystem;
class Entity;
class AnimationComponent;
class CollisionComponent;
class ScriptComponent;
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
    bool shouldSnapToGrid() const;

private:
    struct EditorAnimationFrame {
        int tileId = 0;
        int durationMs = 200;
    };

    struct EditorTileAnimation {
        std::vector<EditorAnimationFrame> frames;
    };

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
        std::unordered_map<int, EditorTileAnimation> animations;
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
    void drawAssetsMenu();
    void drawCreateTilesetPopup();
    void refreshTilesets();
    void drawTilesetSelector();
    void drawSelectedTilesetGrid();
    void drawSelectedTileAnimationEditor(EditorTileset& tileset);
    void drawLevelDropTarget();
    void drawEntityTreeNode(Entity& entity);
    void drawEntityComponents(Entity& entity);
    void drawEntityIdentityFields(Entity& entity);
    void drawTransformComponentFields(TransformComponent& transform);
    void drawSpriteComponentFields(SpriteComponent& spriteComponent);
    void drawAnimationComponentFields(AnimationComponent& animationComponent);
    void drawCollisionComponentFields(CollisionComponent& collisionComponent);
    void drawScriptComponentFields(ScriptComponent& scriptComponent);
    void handleViewportCameraZoom();
    void handleViewportCameraPan();
    bool isViewportCameraPanActive() const;
    void handleViewportEntityInteraction();
    Entity* findEntityAt(float x, float y);
    Entity* findEntityById(int id);
    bool entityContainsPoint(Entity& entity, float x, float y) const;
    RenderRect getViewportForCamera() const;
    void syncEditStateFromEntity(Entity& entity, bool force = false);
    void createSpriteEntityFromTile(int tilesetIndex, int tileId, float x, float y);
    void drawCreateTilesetImagePreview(TextureAsset& textureAsset);
    bool createTilesetFromTexture(
        TextureAsset& textureAsset,
        const std::string& tilesetName,
        int tileWidth,
        int tileHeight,
        bool useTransparencyColor,
        const float transparencyColor[3]
    );
    bool loadCreateTilesetPreviewImage(TextureAsset& textureAsset);
    void resetCreateTilesetPreviewImage();
    void resetCreateTilesetForm();
    bool saveTilesetAnimations(EditorTileset& tileset);
    void drawViewportGrid();

    bool enabled = true;
    bool showGrid = true;
    bool snapToGrid = false;
    bool showColliders = false;
    bool tilesetsScanned = false;
    bool showCreateTilesetPopup = false;
    bool newTilesetUseTransparencyColor = true;
    bool pickingTransparencyColor = false;
    float tilePreviewScale = 3.0f;
    float newTilesetTransparencyColor[3] = {0.0f, 0.0f, 1.0f / 255.0f};
    int selectedTilesetIndex = -1;
    int selectedTileId = -1;
    int selectedEntityId = -1;
    int draggingEntityId = -1;
    int createdSpriteCount = 0;
    int selectedCreateTilesetTextureIndex = -1;
    int newTilesetTileWidth = 16;
    int newTilesetTileHeight = 16;
    int animationOwnerTileId = -1;
    int newAnimationFrameDurationMs = 200;
    bool consumedPinchZoomThisFrame = false;
    Tool currentTool = Tool::Select;
    float dragOffset[2] = {0.0f, 0.0f};
    std::string statusMessage;
    std::string newTilesetName;
    std::string newTilesetOutputFile;
    std::string createTilesetPreviewImagePath;
    int createTilesetPreviewImageWidth = 0;
    int createTilesetPreviewImageHeight = 0;
    EntityEditState entityEditState;
    std::vector<unsigned char> createTilesetPreviewPixels;
    std::vector<EditorTileset> tilesets;
};

#endif
