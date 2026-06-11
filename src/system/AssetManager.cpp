#include "system/AssetManager.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <memory>

namespace {
std::string toLower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });
    return value;
}

Asset::Type typeFromExtension(const std::filesystem::path& path) {
    const std::string extension = toLower(path.extension().string());

    if (extension == ".png") {
        return Asset::Type::Texture;
    }

    if (extension == ".wav") {
        return Asset::Type::Sound;
    }

    if (extension == ".ogg") {
        return Asset::Type::Music;
    }

    if (extension == ".tmx") {
        return Asset::Type::Level;
    }

    return Asset::Type::Unknown;
}

std::unique_ptr<Asset> createAsset(const std::filesystem::path& assetPath, Asset::Type assetType) {
    const std::string name = assetPath.stem().string();
    const std::string path = assetPath.string();

    switch (assetType) {
        case Asset::Type::Texture:
            return std::make_unique<TextureAsset>(name, path);
        case Asset::Type::Sound:
            return std::make_unique<SoundAsset>(name, path);
        case Asset::Type::Music:
            return std::make_unique<MusicAsset>(name, path);
        case Asset::Type::Level:
            return std::make_unique<LevelAsset>(name, path);
        case Asset::Type::Unknown:
        default:
            return nullptr;
    }
}
}

AssetManager& AssetManager::getInstance() {
    static AssetManager instance;
    return instance;
}

bool AssetManager::loadAssets(const std::string& assetsDirectory) {
    namespace fs = std::filesystem;

    assets.clear();

    const fs::path rootPath(assetsDirectory);
    if (!fs::exists(rootPath) || !fs::is_directory(rootPath)) {
        std::cerr << "Assets directory not found: " << assetsDirectory << std::endl;
        return false;
    }

    bool loadedAllKnownAssets = true;

    for (const fs::directory_entry& entry : fs::recursive_directory_iterator(rootPath)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const fs::path assetPath = entry.path();
        const Asset::Type assetType = typeFromExtension(assetPath);

        if (assetType == Asset::Type::Unknown) {
            continue;
        }

        std::unique_ptr<Asset> asset = createAsset(assetPath, assetType);
        if (asset == nullptr) {
            continue;
        }

        loadedAllKnownAssets = asset->load() && loadedAllKnownAssets;
        assets.push_back(std::move(asset));
    }

    std::cout << "Loaded " << assets.size() << " asset(s) from " << assetsDirectory << std::endl;
    return loadedAllKnownAssets;
}

void AssetManager::clearAssets() {
    assets.clear();
}

Asset* AssetManager::getAssetByName(const std::string& name) {
    for (const auto& asset : assets) {
        if (asset->getName() == name || std::filesystem::path(asset->getPath()).filename().string() == name) {
            return asset.get();
        }
    }

    return nullptr;
}

const Asset* AssetManager::getAssetByName(const std::string& name) const {
    for (const auto& asset : assets) {
        if (asset->getName() == name || std::filesystem::path(asset->getPath()).filename().string() == name) {
            return asset.get();
        }
    }

    return nullptr;
}

TextureAsset* AssetManager::getTextureAssetByName(const std::string& name) {
    return dynamic_cast<TextureAsset*>(getAssetByName(name));
}

const TextureAsset* AssetManager::getTextureAssetByName(const std::string& name) const {
    return dynamic_cast<const TextureAsset*>(getAssetByName(name));
}

SoundAsset* AssetManager::getSoundAssetByName(const std::string& name) {
    return dynamic_cast<SoundAsset*>(getAssetByName(name));
}

const SoundAsset* AssetManager::getSoundAssetByName(const std::string& name) const {
    return dynamic_cast<const SoundAsset*>(getAssetByName(name));
}

MusicAsset* AssetManager::getMusicAssetByName(const std::string& name) {
    return dynamic_cast<MusicAsset*>(getAssetByName(name));
}

const MusicAsset* AssetManager::getMusicAssetByName(const std::string& name) const {
    return dynamic_cast<const MusicAsset*>(getAssetByName(name));
}

LevelAsset* AssetManager::getLevelAssetByName(const std::string& name) {
    return dynamic_cast<LevelAsset*>(getAssetByName(name));
}

const LevelAsset* AssetManager::getLevelAssetByName(const std::string& name) const {
    return dynamic_cast<const LevelAsset*>(getAssetByName(name));
}

const std::vector<std::unique_ptr<Asset>>& AssetManager::getAssets() const {
    return assets;
}
