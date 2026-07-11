#include "components/UIEditTextComponent.h"

#include <algorithm>
#include <memory>

const std::string& UIEditTextComponent::getText() const {
    return text;
}

const std::string& UIEditTextComponent::getPlaceholder() const {
    return placeholder;
}

const std::string& UIEditTextComponent::getSubmitFunction() const {
    return submitFunction;
}

const std::string& UIEditTextComponent::getFontName() const {
    return fontName;
}

const RenderColor& UIEditTextComponent::getTextColor() const {
    return textColor;
}

const RenderColor& UIEditTextComponent::getPlaceholderColor() const {
    return placeholderColor;
}

const RenderColor& UIEditTextComponent::getBackgroundColor() const {
    return backgroundColor;
}

const RenderColor& UIEditTextComponent::getFocusedColor() const {
    return focusedColor;
}

const RenderColor& UIEditTextComponent::getCursorColor() const {
    return cursorColor;
}

int UIEditTextComponent::getFontSize() const {
    return fontSize;
}

std::size_t UIEditTextComponent::getCursorIndex() const {
    return cursorIndex;
}

int UIEditTextComponent::getMaxLength() const {
    return maxLength;
}

bool UIEditTextComponent::isFocused() const {
    return focused;
}

bool UIEditTextComponent::isReadOnly() const {
    return readOnly;
}

bool UIEditTextComponent::isPassword() const {
    return password;
}

void UIEditTextComponent::setText(const std::string& value) {
    if (maxLength <= 0) {
        text.clear();
    } else {
        text = value.substr(0, static_cast<std::size_t>(maxLength));
    }

    setCursorIndex(std::min(cursorIndex, text.size()));
}

void UIEditTextComponent::setPlaceholder(const std::string& value) {
    placeholder = value;
}

void UIEditTextComponent::setSubmitFunction(const std::string& value) {
    submitFunction = value;
}

void UIEditTextComponent::setFontName(const std::string& value) {
    fontName = value;
}

void UIEditTextComponent::setTextColor(const RenderColor& value) {
    textColor = value;
}

void UIEditTextComponent::setPlaceholderColor(const RenderColor& value) {
    placeholderColor = value;
}

void UIEditTextComponent::setBackgroundColor(const RenderColor& value) {
    backgroundColor = value;
}

void UIEditTextComponent::setFocusedColor(const RenderColor& value) {
    focusedColor = value;
}

void UIEditTextComponent::setCursorColor(const RenderColor& value) {
    cursorColor = value;
}

void UIEditTextComponent::setFontSize(int value) {
    fontSize = std::max(1, value);
}

void UIEditTextComponent::setCursorIndex(std::size_t value) {
    cursorIndex = std::min(value, text.size());
}

void UIEditTextComponent::setMaxLength(int value) {
    maxLength = std::max(0, value);
    setText(text);
}

void UIEditTextComponent::setFocused(bool value) {
    focused = value;
}

void UIEditTextComponent::setReadOnly(bool value) {
    readOnly = value;
}

void UIEditTextComponent::setPassword(bool value) {
    password = value;
}

void UIEditTextComponent::insertText(const std::string& value) {
    if (readOnly || value.empty() || maxLength <= 0) {
        return;
    }

    const std::size_t available =
        static_cast<std::size_t>(maxLength) > text.size()
            ? static_cast<std::size_t>(maxLength) - text.size()
            : 0;
    if (available == 0) {
        return;
    }

    const std::string inserted = value.substr(0, available);
    text.insert(cursorIndex, inserted);
    cursorIndex += inserted.size();
}

void UIEditTextComponent::backspace() {
    if (readOnly || cursorIndex == 0 || text.empty()) {
        return;
    }

    text.erase(cursorIndex - 1, 1);
    --cursorIndex;
}

void UIEditTextComponent::clear() {
    if (readOnly) {
        return;
    }

    text.clear();
    cursorIndex = 0;
}

std::string UIEditTextComponent::getDisplayText() const {
    if (!password) {
        return text;
    }

    return std::string(text.size(), '*');
}

std::unique_ptr<Component> UIEditTextComponent::clone() const {
    auto copy = std::make_unique<UIEditTextComponent>();
    copy->setMaxLength(maxLength);
    copy->setText(text);
    copy->setPlaceholder(placeholder);
    copy->setSubmitFunction(submitFunction);
    copy->setFontName(fontName);
    copy->setTextColor(textColor);
    copy->setPlaceholderColor(placeholderColor);
    copy->setBackgroundColor(backgroundColor);
    copy->setFocusedColor(focusedColor);
    copy->setCursorColor(cursorColor);
    copy->setFontSize(fontSize);
    copy->setCursorIndex(cursorIndex);
    copy->setFocused(false);
    copy->setReadOnly(readOnly);
    copy->setPassword(password);
    copy->setEnabled(isEnabled());
    return copy;
}
