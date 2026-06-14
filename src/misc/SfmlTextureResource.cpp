#include "misc/SfmlTextureResource.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <iostream>
#include <memory>

SfmlTextureResource::~SfmlTextureResource() = default;

bool SfmlTextureResource::loadFromFile(const std::string& path) {
    sf::Image image;
    if (!image.loadFromFile(path)) {
        std::cerr << "Failed to load texture image: " << path << std::endl;
        return false;
    }

    image.createMaskFromColor(sf::Color(0, 0, 1));

    texture = std::make_unique<sf::Texture>();
    texture->setSmooth(false);

    if (!texture->loadFromImage(image)) {
        std::cerr << "Failed to create SFML texture: " << path << std::endl;
        texture.reset();
        return false;
    }

    return true;
}

RenderTextureHandle SfmlTextureResource::getHandle() const {
    return texture.get();
}
