#ifndef IENGINEV2_TEXTUREASSET_H
#define IENGINEV2_TEXTUREASSET_H

#include "misc/Asset.h"

#include <SFML/Graphics/Texture.hpp>

#include <memory>
#include <string>

class TextureAsset : public Asset {
public:
    TextureAsset(std::string name, std::string path);

    bool load() override;

    sf::Texture* getTexture();
    const sf::Texture* getTexture() const;

private:
    std::unique_ptr<sf::Texture> texture;
};

#endif
