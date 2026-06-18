#include "misc/PrefabAsset.h"

#include "tinyxml2.h"

#include <filesystem>
#include <iostream>
#include <utility>

PrefabAsset::PrefabAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Prefab) {}

bool PrefabAsset::load() {
    if (!std::filesystem::exists(getPath())) {
        std::cerr << "Prefab file does not exist: " << getPath() << std::endl;
        setLoaded(false);
        return false;
    }

    tinyxml2::XMLDocument document;
    if (document.LoadFile(getPath().c_str()) != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load prefab asset: " << getPath()
                  << " - " << document.ErrorStr() << std::endl;
        setLoaded(false);
        return false;
    }

    if (document.FirstChildElement("prefab") == nullptr) {
        std::cerr << "Prefab asset is missing prefab root: " << getPath() << std::endl;
        setLoaded(false);
        return false;
    }

    setLoaded(true);
    return true;
}
