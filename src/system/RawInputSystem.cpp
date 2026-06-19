#include "system/RawInputSystem.h"

#include <algorithm>
#include <iostream>

namespace {
bool getState(const std::unordered_map<RawKey, bool>& states, RawKey key) {
    const auto iterator = states.find(key);
    return iterator != states.end() && iterator->second;
}

bool getState(const std::unordered_map<RawMouseButton, bool>& states, RawMouseButton button) {
    const auto iterator = states.find(button);
    return iterator != states.end() && iterator->second;
}

const char* rawKeyToString(RawKey key) {
    switch (key) {
        case RawKey::W:
            return "W";
        case RawKey::A:
            return "A";
        case RawKey::S:
            return "S";
        case RawKey::D:
            return "D";
        case RawKey::Space:
            return "Space";
        case RawKey::Escape:
            return "Escape";
        case RawKey::Unknown:
        default:
            return "Unknown";
    }
}

const char* rawMouseButtonToString(RawMouseButton button) {
    switch (button) {
        case RawMouseButton::Left:
            return "Left";
        case RawMouseButton::Right:
            return "Right";
        case RawMouseButton::Middle:
            return "Middle";
        case RawMouseButton::Unknown:
        default:
            return "Unknown";
    }
}
}

RawInputSystem& RawInputSystem::getInstance() {
    static RawInputSystem instance;
    return instance;
}

void RawInputSystem::beginFrame() {
    previousKeys = currentKeys;
    previousMouseButtons = currentMouseButtons;
}

void RawInputSystem::endFrame() {
    touches.erase(
        std::remove_if(
            touches.begin(),
            touches.end(),
            [](const RawTouch& touch) {
                return !touch.down;
            }
        ),
        touches.end()
    );
}

void RawInputSystem::setKeyDown(RawKey key) {
    if (key == RawKey::Unknown) {
        return;
    }

    currentKeys[key] = true;
}

void RawInputSystem::setKeyUp(RawKey key) {
    if (key == RawKey::Unknown) {
        return;
    }

    currentKeys[key] = false;
}

void RawInputSystem::setMouseButtonDown(RawMouseButton button, int x, int y) {
    setMousePosition(x, y);
    if (button == RawMouseButton::Unknown) {
        return;
    }

    currentMouseButtons[button] = true;
}

void RawInputSystem::setMouseButtonUp(RawMouseButton button, int x, int y) {
    setMousePosition(x, y);
    if (button == RawMouseButton::Unknown) {
        return;
    }

    currentMouseButtons[button] = false;
}

void RawInputSystem::setMousePosition(int x, int y) {
    mouseX = x;
    mouseY = y;
}

void RawInputSystem::setTouchDown(int touchId, float x, float y) {
    RawTouch* touch = findTouch(touchId);
    if (touch == nullptr) {
        touches.push_back(RawTouch{touchId, x, y, true});
        return;
    }

    touch->x = x;
    touch->y = y;
    touch->down = true;
}

void RawInputSystem::setTouchMove(int touchId, float x, float y) {
    RawTouch* touch = findTouch(touchId);
    if (touch == nullptr) {
        touches.push_back(RawTouch{touchId, x, y, true});
        return;
    }

    touch->x = x;
    touch->y = y;
}

void RawInputSystem::setTouchUp(int touchId, float x, float y) {
    RawTouch* touch = findTouch(touchId);
    if (touch == nullptr) {
        touches.push_back(RawTouch{touchId, x, y, false});
        return;
    }

    touch->x = x;
    touch->y = y;
    touch->down = false;
}

bool RawInputSystem::isKeyDown(RawKey key) const {
    return getState(currentKeys, key);
}

bool RawInputSystem::wasKeyPressed(RawKey key) const {
    return getState(currentKeys, key) && !getState(previousKeys, key);
}

bool RawInputSystem::wasKeyReleased(RawKey key) const {
    return !getState(currentKeys, key) && getState(previousKeys, key);
}

bool RawInputSystem::isMouseButtonDown(RawMouseButton button) const {
    return getState(currentMouseButtons, button);
}

bool RawInputSystem::wasMouseButtonPressed(RawMouseButton button) const {
    return getState(currentMouseButtons, button) && !getState(previousMouseButtons, button);
}

bool RawInputSystem::wasMouseButtonReleased(RawMouseButton button) const {
    return !getState(currentMouseButtons, button) && getState(previousMouseButtons, button);
}

int RawInputSystem::getMouseX() const {
    return mouseX;
}

int RawInputSystem::getMouseY() const {
    return mouseY;
}

const std::vector<RawTouch>& RawInputSystem::getTouches() const {
    return touches;
}

void RawInputSystem::debugPrintState() const {
    std::cout << "RawInputSystem state" << std::endl;
    std::cout << "  Mouse: (" << mouseX << ", " << mouseY << ")" << std::endl;

    std::cout << "  Keys:" << std::endl;
    for (const auto& keyState : currentKeys) {
        std::cout << "    " << rawKeyToString(keyState.first)
                  << " down=" << (keyState.second ? "true" : "false")
                  << " pressed=" << (wasKeyPressed(keyState.first) ? "true" : "false")
                  << " released=" << (wasKeyReleased(keyState.first) ? "true" : "false")
                  << std::endl;
    }

    std::cout << "  Mouse buttons:" << std::endl;
    for (const auto& buttonState : currentMouseButtons) {
        std::cout << "    " << rawMouseButtonToString(buttonState.first)
                  << " down=" << (buttonState.second ? "true" : "false")
                  << " pressed=" << (wasMouseButtonPressed(buttonState.first) ? "true" : "false")
                  << " released=" << (wasMouseButtonReleased(buttonState.first) ? "true" : "false")
                  << std::endl;
    }

    std::cout << "  Touches:" << std::endl;
    for (const RawTouch& touch : touches) {
        std::cout << "    id=" << touch.id
                  << " position=(" << touch.x << ", " << touch.y << ")"
                  << " down=" << (touch.down ? "true" : "false")
                  << std::endl;
    }
}

RawTouch* RawInputSystem::findTouch(int touchId) {
    for (RawTouch& touch : touches) {
        if (touch.id == touchId) {
            return &touch;
        }
    }

    return nullptr;
}

const RawTouch* RawInputSystem::findTouch(int touchId) const {
    for (const RawTouch& touch : touches) {
        if (touch.id == touchId) {
            return &touch;
        }
    }

    return nullptr;
}
