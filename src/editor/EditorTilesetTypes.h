#pragma once

#include <string>
#include <unordered_map>
#include <vector>

class TextureAsset;

namespace EditorTileDrag {
constexpr const char* PayloadType = "IENGINE_TILE";
}

struct TileDragPayload {
    int tilesetIndex = -1;
    int tileId = -1;
};

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
