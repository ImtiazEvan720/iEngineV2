#include "misc/LevelAsset.h"

#include "system/AssetManager.h"

#include <tinyxml2.h>

#include <filesystem>
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

namespace {
std::string getAttribute(const tinyxml2::XMLElement* element, const char* name, const std::string& fallback = "") {
    if (element == nullptr) {
        return fallback;
    }

    const char* value = element->Attribute(name);
    return value == nullptr ? fallback : value;
}

std::filesystem::path resolveRelativePath(const std::filesystem::path& baseFile, const std::string& relativePath) {
    std::filesystem::path path(relativePath);
    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return (baseFile.parent_path() / path).lexically_normal();
}

std::vector<int> parseCsvGids(const char* text) {
    std::vector<int> gids;
    if (text == nullptr) {
        return gids;
    }

    std::stringstream stream(text);
    std::string token;
    while (std::getline(stream, token, ',')) {
        try {
            gids.push_back(std::stoi(token));
        } catch (const std::exception&) {
            gids.push_back(0);
        }
    }

    return gids;
}

std::unordered_map<std::string, std::string> parseProperties(const tinyxml2::XMLElement* parent) {
    std::unordered_map<std::string, std::string> properties;
    const tinyxml2::XMLElement* propertiesElement = parent == nullptr ? nullptr : parent->FirstChildElement("properties");
    if (propertiesElement == nullptr) {
        return properties;
    }

    for (const tinyxml2::XMLElement* property = propertiesElement->FirstChildElement("property");
         property != nullptr;
         property = property->NextSiblingElement("property")) {
        const std::string name = getAttribute(property, "name");
        const std::string value = getAttribute(property, "value");
        if (!name.empty()) {
            properties[name] = value;
        }
    }

    return properties;
}

TileLayerInfo parseTileLayer(const tinyxml2::XMLElement* layer) {
    TileLayerInfo tileLayer;
    tileLayer.name = getAttribute(layer, "name");
    tileLayer.width = layer->IntAttribute("width");
    tileLayer.height = layer->IntAttribute("height");
    tileLayer.visible = layer->IntAttribute("visible", 1) != 0;

    const tinyxml2::XMLElement* data = layer->FirstChildElement("data");
    if (data != nullptr && getAttribute(data, "encoding") == "csv") {
        tileLayer.gids = parseCsvGids(data->GetText());
    }

    return tileLayer;
}

ObjectInfo parseObject(const tinyxml2::XMLElement* object) {
    ObjectInfo info;
    info.name = getAttribute(object, "name");
    info.type = getAttribute(object, "type");
    info.x = object->FloatAttribute("x");
    info.y = object->FloatAttribute("y");
    info.width = object->FloatAttribute("width");
    info.height = object->FloatAttribute("height");
    info.rotation = object->FloatAttribute("rotation");
    info.properties = parseProperties(object);
    return info;
}

void appendObjects(const tinyxml2::XMLElement* objectGroup, std::vector<ObjectInfo>& objects) {
    for (const tinyxml2::XMLElement* object = objectGroup->FirstChildElement("object");
         object != nullptr;
         object = object->NextSiblingElement("object")) {
        objects.push_back(parseObject(object));
    }
}
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

    const tinyxml2::XMLElement* map = document.FirstChildElement("map");
    if (map == nullptr) {
        std::cerr << "Level asset is missing map root: " << getPath() << std::endl;
        setLoaded(false);
        return false;
    }

    mapWidth = map->IntAttribute("width");
    mapHeight = map->IntAttribute("height");
    tileWidth = map->IntAttribute("tilewidth");
    tileHeight = map->IntAttribute("tileheight");

    const std::filesystem::path mapPath(getPath());

    for (const tinyxml2::XMLElement* tilesetElement = map->FirstChildElement("tileset");
         tilesetElement != nullptr;
         tilesetElement = tilesetElement->NextSiblingElement("tileset")) {
        TilesetInfo tileset;
        tileset.firstGid = tilesetElement->IntAttribute("firstgid");

        const std::string source = getAttribute(tilesetElement, "source");
        const tinyxml2::XMLElement* tilesetRoot = tilesetElement;
        tinyxml2::XMLDocument tilesetDocument;

        if (!source.empty()) {
            tileset.tsxPath = resolveRelativePath(mapPath, source).string();
            if (tilesetDocument.LoadFile(tileset.tsxPath.c_str()) != tinyxml2::XML_SUCCESS) {
                std::cerr << "Failed to load tileset: " << tileset.tsxPath
                          << " - " << tilesetDocument.ErrorStr() << std::endl;
                setLoaded(false);
                return false;
            }

            tilesetRoot = tilesetDocument.FirstChildElement("tileset");
            if (tilesetRoot == nullptr) {
                std::cerr << "Tileset is missing root: " << tileset.tsxPath << std::endl;
                setLoaded(false);
                return false;
            }
        }

        tileset.name = getAttribute(tilesetRoot, "name");
        tileset.tileWidth = tilesetRoot->IntAttribute("tilewidth");
        tileset.tileHeight = tilesetRoot->IntAttribute("tileheight");
        tileset.tileCount = tilesetRoot->IntAttribute("tilecount");
        tileset.columns = tilesetRoot->IntAttribute("columns");

        const tinyxml2::XMLElement* image = tilesetRoot->FirstChildElement("image");
        if (image != nullptr) {
            const std::string imageSource = getAttribute(image, "source");
            const std::filesystem::path sourceBase = tileset.tsxPath.empty() ? mapPath : std::filesystem::path(tileset.tsxPath);
            tileset.imagePath = resolveRelativePath(sourceBase, imageSource).string();
            const std::string imageFilename = std::filesystem::path(imageSource).filename().string();
            tileset.textureAsset = AssetManager::getInstance().getTextureAssetByName(imageFilename);

            if (tileset.textureAsset == nullptr) {
                std::cerr << "Tileset image has no loaded TextureAsset: " << imageFilename << std::endl;
                setLoaded(false);
                return false;
            }
        }

        for (const tinyxml2::XMLElement* tile = tilesetRoot->FirstChildElement("tile");
             tile != nullptr;
             tile = tile->NextSiblingElement("tile")) {
            const int tileId = tile->IntAttribute("id");
            const tinyxml2::XMLElement* animation = tile->FirstChildElement("animation");
            if (animation == nullptr) {
                continue;
            }

            TileAnimation tileAnimation;
            for (const tinyxml2::XMLElement* frame = animation->FirstChildElement("frame");
                 frame != nullptr;
                 frame = frame->NextSiblingElement("frame")) {
                TileAnimationFrame animationFrame;
                animationFrame.tileId = frame->IntAttribute("tileid");
                animationFrame.durationSeconds = frame->FloatAttribute("duration") / 1000.0f;
                tileAnimation.frames.push_back(animationFrame);
            }

            if (!tileAnimation.frames.empty()) {
                tileset.animations[tileId] = std::move(tileAnimation);
            }
        }

        tilesets.push_back(std::move(tileset));
    }

    for (const tinyxml2::XMLElement* groupElement = map->FirstChildElement("group");
         groupElement != nullptr;
         groupElement = groupElement->NextSiblingElement("group")) {
        LevelGroupInfo group;
        group.name = getAttribute(groupElement, "name");

        for (const tinyxml2::XMLElement* child = groupElement->FirstChildElement();
             child != nullptr;
             child = child->NextSiblingElement()) {
            const std::string elementName = child->Name();

            if (elementName == "layer") {
                group.tileLayers.push_back(parseTileLayer(child));
            } else if (elementName == "objectgroup") {
                appendObjects(child, group.objects);
            }
        }

        groups.push_back(std::move(group));
    }

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
