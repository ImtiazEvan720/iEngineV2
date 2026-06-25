#pragma once

#include "editor/SpritePickerWidget.h"
#include "system/IRenderBackend.h"

#include <string>

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
    void drawSelectedTilesetGrid(std::string& statusMessage);
    void drawSelectedTileAnimationEditor(EditorTileset& tileset, std::string& statusMessage);
    void createSpriteEntityFromTile(int tilesetIndex, int tileId, float x, float y, std::string& statusMessage);
    bool saveTilesetAnimations(EditorTileset& tileset, std::string& statusMessage);

    SpritePickerWidget spritePicker;
    int createdSpriteCount = 0;
    int animationOwnerTileId = -1;
    int newAnimationFrameDurationMs = 200;
};
