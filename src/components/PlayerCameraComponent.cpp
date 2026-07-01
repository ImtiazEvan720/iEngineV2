#include "components/PlayerCameraComponent.h"

#include <algorithm>

PlayerCameraComponent::PlayerCameraComponent(
    float zoom,
    float viewportWidth,
    float viewportHeight,
    const Vector2F& offset,
    bool clampToBounds,
    float minX,
    float minY,
    float maxX,
    float maxY
)
    : zoom(zoom),
      viewportWidth(viewportWidth),
      viewportHeight(viewportHeight),
      offset(offset),
      clampToBounds(clampToBounds),
      minX(minX),
      minY(minY),
      maxX(maxX),
      maxY(maxY) {}

float PlayerCameraComponent::getZoom() const {
    return zoom;
}

float PlayerCameraComponent::getViewportWidth() const {
    return viewportWidth;
}

float PlayerCameraComponent::getViewportHeight() const {
    return viewportHeight;
}

const Vector2F& PlayerCameraComponent::getOffset() const {
    return offset;
}

bool PlayerCameraComponent::shouldClampToBounds() const {
    return clampToBounds;
}

float PlayerCameraComponent::getMinX() const {
    return minX;
}

float PlayerCameraComponent::getMinY() const {
    return minY;
}

float PlayerCameraComponent::getMaxX() const {
    return maxX;
}

float PlayerCameraComponent::getMaxY() const {
    return maxY;
}

void PlayerCameraComponent::setZoom(float zoom) {
    this->zoom = std::max(0.001f, zoom);
}

void PlayerCameraComponent::setViewportWidth(float viewportWidth) {
    this->viewportWidth = std::max(1.0f, viewportWidth);
}

void PlayerCameraComponent::setViewportHeight(float viewportHeight) {
    this->viewportHeight = std::max(1.0f, viewportHeight);
}

void PlayerCameraComponent::setOffset(const Vector2F& offset) {
    this->offset = offset;
}

void PlayerCameraComponent::setClampToBounds(bool clampToBounds) {
    this->clampToBounds = clampToBounds;
}

void PlayerCameraComponent::setBounds(float minX, float minY, float maxX, float maxY) {
    this->minX = minX;
    this->minY = minY;
    this->maxX = maxX;
    this->maxY = maxY;
}

std::unique_ptr<Component> PlayerCameraComponent::clone() const {
    auto copy = std::make_unique<PlayerCameraComponent>(
        zoom,
        viewportWidth,
        viewportHeight,
        offset,
        clampToBounds,
        minX,
        minY,
        maxX,
        maxY
    );
    copy->setEnabled(isEnabled());
    return copy;
}
