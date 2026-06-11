#include "components/TransformComponent.h"

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

    Vector2F parentPosition = parent->getWorldPosition();
    parentPosition.x += position.x;
    parentPosition.y += position.y;
    return parentPosition;
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

void TransformComponent::setParent(TransformComponent* parent) {
    this->parent = parent;
}

TransformComponent* TransformComponent::getParent() const {
    return parent;
}
