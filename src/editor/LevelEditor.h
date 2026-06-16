#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include <string>
#include <vector>

class InputSystem;
class TextureAsset;

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

    void drawLevelOutlineTab(const InputSystem& inputSystem);
    void drawSpritesTab();
    void drawFileExplorerTab();
    void refreshTilesets();
    void drawTilesetSelector();
    void drawSelectedTilesetGrid();

    bool enabled = true;
    bool showGrid = true;
    bool showColliders = false;
    bool tilesetsScanned = false;
    float tilePreviewScale = 3.0f;
    int selectedTilesetIndex = -1;
    int selectedTileId = -1;
    Tool currentTool = Tool::Select;
    std::vector<EditorTileset> tilesets;
};

#endif
