#include "misc/Sprite.h"

Sprite::Sprite(sf::Texture* texture, const sf::FloatRect& sourceRect)
    : texture(texture),
      sourceRect(sourceRect),
      origin(sourceRect.size.x/2, sourceRect.size.y/2),
      size(sourceRect.size.x, sourceRect.size.y) {}

sf::Texture* Sprite::getTexture() const {
    return texture;
}

const sf::FloatRect& Sprite::getSourceRect() const {
    return sourceRect;
}

const Vector2F& Sprite::getOrigin() const {
    return origin;
}

const Vector2F& Sprite::getSize() const {
    return size;
}

void Sprite::setTexture(sf::Texture* texture) {
    this->texture = texture;
}

void Sprite::setSourceRect(const sf::FloatRect& sourceRect) {
    this->sourceRect = sourceRect;
}

void Sprite::setOrigin(const Vector2F& origin) {
    this->origin = origin;
}

void Sprite::setSize(const Vector2F& size) {
    this->size = size;
}
