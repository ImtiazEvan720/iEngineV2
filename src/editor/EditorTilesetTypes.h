#pragma once

#include <string>
class TextureAsset;

namespace EditorTileDrag {
constexpr const char* PayloadType = "IENGINE_TILE";
}

struct TileDragPayload {
    int tilesetIndex = -1;
    int tileId = -1;
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
};
