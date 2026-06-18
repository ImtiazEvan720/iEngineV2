#include "misc/Asset.h"

#include <utility>

Asset::Asset(std::string name, std::string path, Type type)
    : name(std::move(name)), path(std::move(path)), type(type), loaded(false) {}

const std::string& Asset::getName() const {
    return name;
}

const std::string& Asset::getPath() const {
    return path;
}

Asset::Type Asset::getType() const {
    return type;
}

bool Asset::isLoaded() const {
    return loaded;
}

void Asset::setLoaded(bool loaded) {
    this->loaded = loaded;
}

const char* Asset::typeToString(Type type) {
    switch (type) {
        case Type::Texture:
            return "Texture";
        case Type::Sound:
            return "Sound";
        case Type::Music:
            return "Music";
        case Type::Level:
            return "Level";
        case Type::Prefab:
            return "Prefab";
        case Type::Unknown:
        default:
            return "Unknown";
    }
}
