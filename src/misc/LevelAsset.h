#ifndef IENGINEV2_LEVELASSET_H
#define IENGINEV2_LEVELASSET_H

#include "misc/Asset.h"
#include "misc/TextureAsset.h"

#include <string>
#include <unordered_map>
#include <vector>

struct TileAnimationFrame {
    int tileId = 0;
    float durationSeconds = 0.0f;

    std::string toString() const;
};

struct TileAnimation {
    std::vector<TileAnimationFrame> frames;

    std::string toString() const;
};

struct TilesetInfo {
    int firstGid = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    int columns = 0;
    int tileCount = 0;
    std::string name;
    std::string tsxPath;
    std::string imagePath;
    TextureAsset* textureAsset = nullptr;
    std::unordered_map<int, TileAnimation> animations;

    std::string toString() const;
};

struct TileLayerInfo {
    std::string name;
    int width = 0;
    int height = 0;
    bool visible = true;
    std::vector<int> gids;

    std::string toString() const;
};

struct ObjectInfo {
    std::string name;
    std::string type;
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float rotation = 0.0f;
    std::unordered_map<std::string, std::string> properties;

    std::string toString() const;
};

struct LevelGroupInfo {
    std::string name;
    std::vector<TileLayerInfo> tileLayers;
    std::vector<ObjectInfo> objects;

    std::string toString() const;
};

class LevelAsset : public Asset {
public:
    LevelAsset(std::string name, std::string path);

    bool load() override;

    int getMapWidth() const;
    int getMapHeight() const;
    int getTileWidth() const;
    int getTileHeight() const;

    const std::vector<TilesetInfo>& getTilesets() const;
    const std::vector<LevelGroupInfo>& getGroups() const;

    std::string toString() const;
    void print() const;

private:
    int mapWidth = 0;
    int mapHeight = 0;
    int tileWidth = 0;
    int tileHeight = 0;
    std::vector<TilesetInfo> tilesets;
    std::vector<LevelGroupInfo> groups;
};

#endif
