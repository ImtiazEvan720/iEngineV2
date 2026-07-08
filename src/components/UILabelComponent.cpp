#include "components/UILabelComponent.h"

#include <algorithm>
#include <memory>

void UILabelComponent::setText(const std::string &value) {
    text = value;
}

const std::string &UILabelComponent::getText() const {
    return text;
}

void UILabelComponent::setFont(const std::string& fontName) {
    this->fontName = fontName;
}

const std::string& UILabelComponent::getFontName() const {
    return fontName;
}

void UILabelComponent::setFontSize(int size) {
    fontSize = std::max(1, size);
}

int UILabelComponent::getFontSize() const {
    return fontSize;
}

void UILabelComponent::setFontColor(const RenderColor& color) {
    fontColor = color;
}

const RenderColor& UILabelComponent::getFontColor() const {
    return fontColor;
}

void UILabelComponent::setHorizontalTextAlign(HorizontalTextAlign value) {
    horizontalAlign = value;
}

HorizontalTextAlign UILabelComponent::getHorizontalTextAlign() const {
    return horizontalAlign;
}

void UILabelComponent::setVerticalTextAlign(VerticalTextAlign value) {
    verticalAlign = value;
}

VerticalTextAlign UILabelComponent::getVerticalTextAlign() const {
    return verticalAlign;
}

std::unique_ptr<Component> UILabelComponent::clone() const {
    auto copy = std::make_unique<UILabelComponent>();
    copy->setText(text);
    copy->setFont(fontName);
    copy->setFontSize(fontSize);
    copy->setFontColor(fontColor);
    copy->setHorizontalTextAlign(horizontalAlign);
    copy->setVerticalTextAlign(verticalAlign);
    copy->setEnabled(isEnabled());
    return copy;
}
