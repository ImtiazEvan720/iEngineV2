#pragma once

#include "misc/Asset.h"
#include "misc/LevelAsset.h"
#include "misc/PrefabAsset.h"
#include "misc/TextureAsset.h"

#ifndef IENGINE_SDL_ONLY
#include "misc/MusicAsset.h"
#include "misc/SoundAsset.h"
#else
class MusicAsset;
class SoundAsset;
#endif

#include <memory>
#include <string>
#include <vector>

class AssetManager {
public:
    static AssetManager& getInstance();

    bool loadAssets(const std::string& assetsDirectory = "Assets");
    bool loadAssetFile(const std::string& assetPath);
    void clearAssets();

    Asset* getAssetByName(const std::string& name);
    const Asset* getAssetByName(const std::string& name) const;
    TextureAsset* getTextureAssetByName(const std::string& name);
    const TextureAsset* getTextureAssetByName(const std::string& name) const;
    SoundAsset* getSoundAssetByName(const std::string& name);
    const SoundAsset* getSoundAssetByName(const std::string& name) const;
    MusicAsset* getMusicAssetByName(const std::string& name);
    const MusicAsset* getMusicAssetByName(const std::string& name) const;
    LevelAsset* getLevelAssetByName(const std::string& name);
    const LevelAsset* getLevelAssetByName(const std::string& name) const;
    PrefabAsset* getPrefabAssetByName(const std::string& name);
    const PrefabAsset* getPrefabAssetByName(const std::string& name) const;
    std::vector<PrefabAsset*> getPrefabAssets();
    std::vector<const PrefabAsset*> getPrefabAssets() const;

    const std::vector<std::unique_ptr<Asset>>& getAssets() const;

private:
    AssetManager() = default;

    std::vector<std::unique_ptr<Asset>> assets;
};
