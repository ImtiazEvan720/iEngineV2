#include "misc/SfmlTextureResource.h"

#include <SFML/Graphics/Image.hpp>
#include <SFML/Graphics/Texture.hpp>

#include <cstring>
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

ImTextureID SfmlTextureResource::getImGuiTextureId() const {
    ImTextureID textureId{};
    if (texture == nullptr) {
        return textureId;
    }

    const auto nativeHandle = texture->getNativeHandle();
    static_assert(sizeof(nativeHandle) <= sizeof(ImTextureID),
                  "ImTextureID is not large enough for an SFML texture handle.");
    std::memcpy(&textureId, &nativeHandle, sizeof(nativeHandle));
    return textureId;
}
