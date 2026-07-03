#include "components/CanvasComponent.h"

void CanvasComponent::setReferenceResolution(const Vector2F& resolution) {
    ReferenceResolution = resolution;
}

void CanvasComponent::setSortingOrder(int order) {
    SortingOrder = order;
}

void CanvasComponent::setOpacity(float opacity) {
    Opacity = opacity;
}

void CanvasComponent::setScale(float scale) {
    Scale = scale;
}

void CanvasComponent::setCanvasColor(const RenderColor& color) {
    CanvasColor = color;
}

const Vector2F& CanvasComponent::getReferenceResolution() const {
    return ReferenceResolution;
}

int CanvasComponent::getSortingOrder() const {
    return SortingOrder;
}

float CanvasComponent::getOpacity() const {
    return Opacity;
}

float CanvasComponent::getScale() const {
    return Scale;
}

const RenderColor& CanvasComponent::getCanvasColor() const {
    return CanvasColor;
}
