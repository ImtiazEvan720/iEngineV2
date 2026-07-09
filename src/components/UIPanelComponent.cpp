#include "components/UIPanelComponent.h"

#include <algorithm>
#include <memory>

void UIPanelComponent::setColor(const RenderColor& value) {
    color = value;
}

const RenderColor& UIPanelComponent::getColor() const {
    return color;
}

void UIPanelComponent::setOpacity(float value) {
    opacity = std::clamp(value, 0.0f, 1.0f);
}

float UIPanelComponent::getOpacity() const {
    return opacity;
}

std::unique_ptr<Component> UIPanelComponent::clone() const {
    auto copy = std::make_unique<UIPanelComponent>();
    copy->setColor(color);
    copy->setOpacity(opacity);
    copy->setEnabled(isEnabled());
    return copy;
}
