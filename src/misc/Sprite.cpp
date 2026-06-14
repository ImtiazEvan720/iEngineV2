#include "misc/Sprite.h"

Sprite::Sprite(RenderTextureHandle texture, const RenderRect& sourceRect)
    : texture(texture),
      sourceRect(sourceRect),
      origin(sourceRect.width / 2.0f, sourceRect.height / 2.0f),
      size(sourceRect.width, sourceRect.height) {}

RenderTextureHandle Sprite::getTextureHandle() const {
    return texture;
}

const RenderRect& Sprite::getSourceRect() const {
    return sourceRect;
}

const Vector2F& Sprite::getOrigin() const {
    return origin;
}

const Vector2F& Sprite::getSize() const {
    return size;
}

void Sprite::setTextureHandle(RenderTextureHandle texture) {
    this->texture = texture;
}

void Sprite::setSourceRect(const RenderRect& sourceRect) {
    this->sourceRect = sourceRect;
}

void Sprite::setOrigin(const Vector2F& origin) {
    this->origin = origin;
}

void Sprite::setSize(const Vector2F& size) {
    this->size = size;
}
