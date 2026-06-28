#pragma once

#include "editor/EditorTilesetTypes.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"

#include <optional>
#include <string>
#include <vector>

class SpritePickerWidget {
public:
    void draw(std::string& statusMessage);
    void refreshTilesets();

    bool drawTilesetSelector();
    bool drawSelectedTilesetDetails(std::string& statusMessage);
    bool drawTileGrid(
        const char* childId,
        bool allowDragDrop,
        int animationOwnerTileId = -1,
        float height = 0.0f,
        bool animatedTilesOnly = false
    );

    std::optional<Sprite> createSpriteFromSelectedTile(float renderScale, std::string& statusMessage) const;
    std::optional<Sprite> createSpriteFromTile(
        int tilesetIndex,
        int tileId,
        float renderScale,
        std::string& statusMessage
    ) const;
    std::optional<Animation> createAnimationFromSelectedTile(float renderScale, std::string& statusMessage) const;
    std::optional<Animation> createAnimationFromTile(
        int tilesetIndex,
        int ownerTileId,
        float renderScale,
        std::string& statusMessage
    ) const;

    bool hasScannedTilesets() const;
    int getSelectedTilesetIndex() const;
    int getSelectedTileId() const;
    bool setSelectedTilesetByFilename(const std::string& filename);
    void setSelectedTileId(int tileId);

    EditorTileset* getSelectedTileset();
    const EditorTileset* getSelectedTileset() const;
    EditorTileset* getTileset(int tilesetIndex);
    const EditorTileset* getTileset(int tilesetIndex) const;
    std::vector<EditorTileset>& getTilesets();
    const std::vector<EditorTileset>& getTilesets() const;

private:
    bool isSelectedTilesetUsable() const;

    bool tilesetsScanned = false;
    float tilePreviewScale = 2.0f;
    int selectedTilesetIndex = -1;
    int selectedTileId = -1;
    std::vector<EditorTileset> tilesets;
};
