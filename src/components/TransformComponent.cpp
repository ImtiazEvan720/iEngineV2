#include "components/TransformComponent.h"

#include <cmath>

TransformComponent::TransformComponent()
    : position(Vector2F::zero()), rotation(0.0f) {}

TransformComponent::TransformComponent(const Vector2F& position, float rotation)
    : position(position), rotation(rotation) {}

const Vector2F& TransformComponent::getPosition() const {
    return position;
}

float TransformComponent::getRotation() const {
    return rotation;
}

Vector2F TransformComponent::getWorldPosition() const {
    if (parent == nullptr) {
        return position;
    }

    constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;

    const Vector2F parentWorldPosition = parent->getWorldPosition();
    const float parentWorldRotation = parent->getWorldRotation();
    const float radians = parentWorldRotation * degreesToRadians;
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);

    const Vector2F rotatedLocalPosition(
        position.x * cosAngle - position.y * sinAngle,
        position.x * sinAngle + position.y * cosAngle
    );

    return Vector2F(
        parentWorldPosition.x + rotatedLocalPosition.x,
        parentWorldPosition.y + rotatedLocalPosition.y
    );
}

float TransformComponent::getWorldRotation() const {
    if (parent == nullptr) {
        return rotation;
    }

    return parent->getWorldRotation() + rotation;
}

void TransformComponent::setPosition(const Vector2F& position) {
    this->position = position;
}

void TransformComponent::setRotation(float rotation) {
    this->rotation = rotation;
}

void TransformComponent::setWorldPosition(const Vector2F& worldPosition) {
    if (parent == nullptr) {
        position = worldPosition;
        return;
    }

    constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;

    const Vector2F parentWorldPosition = parent->getWorldPosition();
    const float parentWorldRotation = parent->getWorldRotation();
    const float radians = parentWorldRotation * degreesToRadians;
    const float cosAngle = std::cos(radians);
    const float sinAngle = std::sin(radians);

    const Vector2F delta(
        worldPosition.x - parentWorldPosition.x,
        worldPosition.y - parentWorldPosition.y
    );

    position = Vector2F(
        delta.x * cosAngle + delta.y * sinAngle,
        -delta.x * sinAngle + delta.y * cosAngle
    );
}

void TransformComponent::setWorldRotation(float worldRotation) {
    if (parent == nullptr) {
        rotation = worldRotation;
        return;
    }

    rotation = worldRotation - parent->getWorldRotation();
}

void TransformComponent::setParent(TransformComponent* parent) {
    this->parent = parent;
}

TransformComponent* TransformComponent::getParent() const {
    return parent;
}

std::unique_ptr<Component> TransformComponent::clone() const {
    auto copy = std::make_unique<TransformComponent>(position, rotation);
    copy->setEnabled(isEnabled());
    return copy;
}
