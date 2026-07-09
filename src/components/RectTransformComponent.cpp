#include "components/RectTransformComponent.h"

#include "math/Math2D.h"

#include <memory>

RectTransformComponent::RectTransformComponent()
    : anchorPosition(Vector2F::zero()), size(Vector2F::zero()), pivot(Vector2F::zero()), rotation(0.0f)
{
}

RectTransformComponent::RectTransformComponent(const Vector2F& anchorPos, const Vector2F& sizeVal, const Vector2F& pivotVal, float rotationVal)
    : anchorPosition(anchorPos), size(sizeVal), pivot(pivotVal), rotation(rotationVal)
{
}

void RectTransformComponent::setAnchoredPosition(const Vector2F& anchorPos) {
    anchorPosition = anchorPos;
}

void RectTransformComponent::setWorldPosition(const Vector2F& worldPosition) {
    if (parent == nullptr) {
        anchorPosition = worldPosition;
        return;
    }

    const Vector2F delta = worldPosition - parent->getWorldPosition();
    anchorPosition = Math2D::inverseRotate(delta, parent->getWorldRotation());
}

void RectTransformComponent::setSize(const Vector2F& sizeVal) {
    size = sizeVal;
}

void RectTransformComponent::setPivot(const Vector2F& pivotVal) {
    pivot = pivotVal;
}

void RectTransformComponent::setRotation(float rotationVal) {
    rotation = rotationVal;
}

void RectTransformComponent::setWorldRotation(float worldRotation) {
    rotation = parent == nullptr
        ? worldRotation
        : worldRotation - parent->getWorldRotation();
}

void RectTransformComponent::setParent(RectTransformComponent* parent) {
    this->parent = parent;
}

const Vector2F& RectTransformComponent::getAnchoredPosition() const {
    return anchorPosition;
}

Vector2F RectTransformComponent::getWorldPosition() const {
    if (parent == nullptr) {
        return anchorPosition;
    }

    const Vector2F rotatedLocalPosition =
        Math2D::rotate(anchorPosition, parent->getWorldRotation());

    return parent->getWorldPosition() + rotatedLocalPosition;
}

const Vector2F& RectTransformComponent::getSize() const {
    return size;
}

const Vector2F& RectTransformComponent::getPivot() const {
    return pivot;
}

float RectTransformComponent::getRotation() const {
    return rotation;
}

float RectTransformComponent::getWorldRotation() const {
    return parent == nullptr
        ? rotation
        : parent->getWorldRotation() + rotation;
}

RectTransformComponent* RectTransformComponent::getParent() {
    return parent;
}

const RectTransformComponent* RectTransformComponent::getParent() const {
    return parent;
}

std::unique_ptr<Component> RectTransformComponent::clone() const {
    auto copy = std::make_unique<RectTransformComponent>(
        anchorPosition,
        size,
        pivot,
        rotation
    );
    copy->setEnabled(isEnabled());
    return copy;
}
