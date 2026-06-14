#include "misc/TextureAsset.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <iostream>
#include <memory>
#include <utility>

TextureAsset::TextureAsset(std::string name, std::string path)
    : Asset(std::move(name), std::move(path), Type::Texture) {}

TextureAsset::~TextureAsset() = default;

bool TextureAsset::load() {
    sf::Image image;
    if (!image.loadFromFile(getPath())) {
        setLoaded(false);
        std::cerr << "Failed to load " << typeToString(getType()) << ": " << getPath() << std::endl;
        return false;
    }

    image.createMaskFromColor(sf::Color(0, 0, 1));

    texture = std::make_unique<sf::Texture>();
    texture->setSmooth(false);

    setLoaded(texture->loadFromImage(image));
    if (!isLoaded()) {
        std::cerr << "Failed to load " << typeToString(getType()) << ": " << getPath() << std::endl;
    }

    return isLoaded();
}

RenderTextureHandle TextureAsset::getTextureHandle() const {
    return texture.get();
}
