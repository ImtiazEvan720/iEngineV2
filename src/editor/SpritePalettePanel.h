#ifndef IENGINEV2_SPRITEPALETTEPANEL_H
#define IENGINEV2_SPRITEPALETTEPANEL_H

#include "editor/EditorTilesetTypes.h"
#include "system/IRenderBackend.h"

#include <string>
#include <vector>

class ViewportGrid;

class SpritePalettePanel {
public:
    void draw(std::string& statusMessage);
    void refreshTilesets();
    void drawLevelDropTarget(
        const ViewportGrid& viewportGrid,
        const RenderRect& viewport,
        float toolbarHeight,
        std::string& statusMessage
    );

private:
    void drawTilesetSelector();
    void drawSelectedTilesetGrid(std::string& statusMessage);
    void drawSelectedTileAnimationEditor(EditorTileset& tileset, std::string& statusMessage);
    void createSpriteEntityFromTile(int tilesetIndex, int tileId, float x, float y, std::string& statusMessage);
    bool saveTilesetAnimations(EditorTileset& tileset, std::string& statusMessage);

    bool tilesetsScanned = false;
    float tilePreviewScale = 3.0f;
    int selectedTilesetIndex = -1;
    int selectedTileId = -1;
    int createdSpriteCount = 0;
    int animationOwnerTileId = -1;
    int newAnimationFrameDurationMs = 200;
    std::vector<EditorTileset> tilesets;
};

#endif
