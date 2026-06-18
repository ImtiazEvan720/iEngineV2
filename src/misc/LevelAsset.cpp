#include "misc/LevelAsset.h"

#include <tinyxml2.h>

#include <iostream>
#include <sstream>
#include <utility>

std::string TileAnimationFrame::toString() const {
    std::ostringstream output;
    output << "TileAnimationFrame(tileId=" << tileId
           << ", durationSeconds=" << durationSeconds << ")";
    return output.str();
}

std::string TileAnimation::toString() const {
    std::ostringstream output;
    output << "TileAnimation(frameCount=" << frames.size() << ")";
    return output.str();
}

std::string TilesetInfo::toString() const {
    std::ostringstream output;
    output << "TilesetInfo(name=\"" << name
           << "\", firstGid=" << firstGid
           << ", tileSize=" << tileWidth << "x" << tileHeight
           << ", columns=" << columns
           << ", tileCount=" << tileCount
           << ", animations=" << animations.size()
           << ", textureLoaded=" << (textureAsset != nullptr ? "true" : "false")
           << ")";
    return output.str();
}

std::string TileLayerInfo::toString() const {
    std::ostringstream output;
    output << "TileLayerInfo(name=\"" << name
           << "\", size=" << width << "x" << height
           << ", visible=" << (visible ? "true" : "false")
           << ", gidCount=" << gids.size()
           << ")";
    return output.str();
}

std::string ObjectInfo::toString() const {
    std::ostringstream output;
    output << "ObjectInfo(name=\"" << name
           << "\", type=\"" << type
           << "\", position=(" << x << ", " << y << ")"
           << ", size=" << width << "x" << height
           << ", rotation=" << rotation
           << ", properties=" << properties.size()
           << ")";
    return output.str();
}

std::string LevelGroupInfo::toString() const {
    std::ostringstream output;
    output << "LevelGroupInfo(name=\"" << name
           << "\", tileLayers=" << tileLayers.size()
           << ", objects=" << objects.size()
           << ")";
    return output.str();
}

LevelAsset::LevelAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Level) {}

bool LevelAsset::load() {
    tilesets.clear();
    groups.clear();

    tinyxml2::XMLDocument document;
    const tinyxml2::XMLError result = document.LoadFile(getPath().c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load level asset: " << getPath() << " - " << document.ErrorStr() << std::endl;
        setLoaded(false);
        return false;
    }

    const tinyxml2::XMLElement* level = document.FirstChildElement("level");
    if (level == nullptr) {
        std::cerr << "Level asset is missing level root: " << getPath() << std::endl;
        setLoaded(false);
        return false;
    }

    mapWidth = 0;
    mapHeight = 0;
    tileWidth = 0;
    tileHeight = 0;

    setLoaded(true);
    return true;
}

int LevelAsset::getMapWidth() const {
    return mapWidth;
}

int LevelAsset::getMapHeight() const {
    return mapHeight;
}

int LevelAsset::getTileWidth() const {
    return tileWidth;
}

int LevelAsset::getTileHeight() const {
    return tileHeight;
}

const std::vector<TilesetInfo>& LevelAsset::getTilesets() const {
    return tilesets;
}

const std::vector<LevelGroupInfo>& LevelAsset::getGroups() const {
    return groups;
}

std::string LevelAsset::toString() const {
    std::ostringstream output;
    output << "LevelAsset(name=\"" << getName()
           << "\", path=\"" << getPath()
           << "\", mapSize=" << mapWidth << "x" << mapHeight
           << ", tileSize=" << tileWidth << "x" << tileHeight
           << ", tilesets=" << tilesets.size()
           << ", groups=" << groups.size()
           << ")";
    return output.str();
}

void LevelAsset::print() const {
    std::cout << toString() << std::endl;

    for (const TilesetInfo& tileset : tilesets) {
        std::cout << "  " << tileset.toString() << std::endl;
        std::cout << "    tsxPath: " << tileset.tsxPath << std::endl;
        std::cout << "    imagePath: " << tileset.imagePath << std::endl;

        for (const auto& animationPair : tileset.animations) {
            std::cout << "    animation tileId=" << animationPair.first
                      << " " << animationPair.second.toString() << std::endl;

            for (const TileAnimationFrame& frame : animationPair.second.frames) {
                std::cout << "      " << frame.toString() << std::endl;
            }
        }
    }

    for (const LevelGroupInfo& group : groups) {
        std::cout << "  " << group.toString() << std::endl;

        for (const TileLayerInfo& tileLayer : group.tileLayers) {
            std::cout << "    " << tileLayer.toString() << std::endl;
        }

        for (const ObjectInfo& object : group.objects) {
            std::cout << "    " << object.toString() << std::endl;

            for (const auto& property : object.properties) {
                std::cout << "      property " << property.first
                          << " = " << property.second << std::endl;
            }
        }
    }
}
