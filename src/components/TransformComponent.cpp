#include "components/TransformComponent.h"

#include "math/Math2D.h"

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

    const Vector2F parentWorldPosition = parent->getWorldPosition();
    const float parentWorldRotation = parent->getWorldRotation();
    const Vector2F rotatedLocalPosition =
        Math2D::rotate(position, parentWorldRotation);

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

    const Vector2F parentWorldPosition = parent->getWorldPosition();
    const float parentWorldRotation = parent->getWorldRotation();
    position = Math2D::inverseRotate(
        worldPosition - parentWorldPosition,
        parentWorldRotation
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
