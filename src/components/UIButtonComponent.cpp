#include "components/UIButtonComponent.h"

void UIButtonComponent::setAction(const std::string& value) {
    action = value;
}

const std::string& UIButtonComponent::getAction() const {
    return action;
}

void UIButtonComponent::setFocus(bool value) {
    focused = value;
}

bool UIButtonComponent::isFocused() const {
    return focused;
}

void UIButtonComponent::setHovered(bool value) {
    hovered = value;
}

bool UIButtonComponent::isHovered() const {
    return hovered;
}

void UIButtonComponent::setInteractable(bool value) {
    interactable = value;
    if (!interactable) {
        hovered = false;
        pressed = false;
        clickedThisFrame = false;
    }
}

bool UIButtonComponent::isInteractable() const {
    return interactable;
}

bool UIButtonComponent::wasClickedThisFrame() const {
    return clickedThisFrame;
}

void UIButtonComponent::setClickedThisFrame(bool value) {
    clickedThisFrame = value;
}

void UIButtonComponent::setPressed(bool value) {
    pressed = value;
}

bool UIButtonComponent::isPressed() const {
    return pressed;
}

const RenderColor& UIButtonComponent::getNormalColor() const {
    return normalColor;
}

const RenderColor& UIButtonComponent::getHoverColor() const {
    return hoverColor;
}

const RenderColor& UIButtonComponent::getPressedColor() const {
    return pressedColor;
}

const RenderColor& UIButtonComponent::getDisabledColor() const {
    return disabledColor;
}

void UIButtonComponent::setNormalColor(const RenderColor& value) {
    normalColor = value;
}

void UIButtonComponent::setHoverColor(const RenderColor& value) {
    hoverColor = value;
}

void UIButtonComponent::setPressedColor(const RenderColor& value) {
    pressedColor = value;
}

void UIButtonComponent::setDisabledColor(const RenderColor& value) {
    disabledColor = value;
}

UIButtonState UIButtonComponent::getState() const {
    if (!interactable || !isEnabled()) {
        return UIButtonState::Disabled;
    }

    if (pressed) {
        return UIButtonState::Pressed;
    }

    if (hovered) {
        return UIButtonState::Hovered;
    }

    return UIButtonState::Normal;
}

RenderColor UIButtonComponent::getCurrentColor() const {
    switch (getState()) {
        case UIButtonState::Disabled:
            return disabledColor;
        case UIButtonState::Pressed:
            return pressedColor;
        case UIButtonState::Hovered:
            return hoverColor;
        case UIButtonState::Normal:
        default:
            return normalColor;
    }
}

void UIButtonComponent::resetFrameState() {
    clickedThisFrame = false;
}

std::unique_ptr<Component> UIButtonComponent::clone() const {
    auto copy = std::make_unique<UIButtonComponent>();
    copy->setAction(action);
    copy->setFocus(focused);
    copy->setHovered(hovered);
    copy->setInteractable(interactable);
    copy->setPressed(pressed);
    copy->setClickedThisFrame(false);
    copy->setNormalColor(normalColor);
    copy->setHoverColor(hoverColor);
    copy->setPressedColor(pressedColor);
    copy->setDisabledColor(disabledColor);
    copy->setEnabled(isEnabled());
    return copy;
}
