#include "components/SpriteComponent.h"

SpriteComponent::SpriteComponent(const Sprite& sprite)
    : sprite(sprite) {}

Sprite& SpriteComponent::getSprite() {
    return sprite;
}

const Sprite& SpriteComponent::getSprite() const {
    return sprite;
}

void SpriteComponent::setSprite(const Sprite& sprite) {
    this->sprite = sprite;
}
