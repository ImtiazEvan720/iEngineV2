#ifndef IENGINEV2_TEXTUREASSET_H
#define IENGINEV2_TEXTUREASSET_H

#include "misc/Asset.h"
#include "system/IRenderBackend.h"

#include <memory>
#include <string>

namespace sf {
class Texture;
}

class TextureAsset : public Asset {
public:
    TextureAsset(std::string name, std::string path);
    ~TextureAsset() override;

    bool load() override;

    RenderTextureHandle getTextureHandle() const;

private:
    std::unique_ptr<sf::Texture> texture;
};

#endif
