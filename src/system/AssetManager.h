#ifndef IENGINEV2_ASSETMANAGER_H
#define IENGINEV2_ASSETMANAGER_H

#include "misc/Asset.h"
#include "misc/LevelAsset.h"
#include "misc/MusicAsset.h"
#include "misc/SoundAsset.h"
#include "misc/TextureAsset.h"

#include <memory>
#include <string>
#include <vector>

class AssetManager {
public:
    static AssetManager& getInstance();

    bool loadAssets(const std::string& assetsDirectory = "Assets");
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

    const std::vector<std::unique_ptr<Asset>>& getAssets() const;

private:
    AssetManager() = default;

    std::vector<std::unique_ptr<Asset>> assets;
};

#endif
