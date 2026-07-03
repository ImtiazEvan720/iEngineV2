#include "components/RectTransformComponent.h"

RectTransformComponent::RectTransformComponent()
    : anchorPosition(Vector2F::zero()), size(Vector2F::zero()), pivot(Vector2F::zero()), rotation(0.0f)
{
}

RectTransformComponent::RectTransformComponent(const Vector2F& anchorPos, const Vector2F& sizeVal, const Vector2F& pivotVal, float rotationVal)
    : anchorPosition(anchorPos), size(sizeVal), pivot(pivotVal), rotation(rotationVal)
{
}

void RectTransformComponent::setAnchoredPosition(const Vector2F anchorPos) {
    anchorPosition = anchorPos;
}

void RectTransformComponent::setSize(const Vector2F sizeVal) {
    size = sizeVal;
}

void RectTransformComponent::setPivot(const Vector2F pivotVal) {
    pivot = pivotVal;
}

void RectTransformComponent::setRotation(float rotationVal) {
    rotation = rotationVal;
}

const Vector2F& RectTransformComponent::getAnchoredPosition() const {
    return anchorPosition;
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
