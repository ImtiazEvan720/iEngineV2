#include "system/InputSystem.h"

#include <algorithm>
#include <sstream>

namespace {
const char* keyToString(InputKey key) {
    switch (key) {
        case InputKey::W:
            return "W";
        case InputKey::A:
            return "A";
        case InputKey::S:
            return "S";
        case InputKey::D:
            return "D";
        case InputKey::Space:
            return "Space";
        case InputKey::Unknown:
        default:
            return "Unknown";
    }
}

const char* mouseButtonToString(InputMouseButton button) {
    switch (button) {
        case InputMouseButton::Left:
            return "Left";
        case InputMouseButton::Right:
            return "Right";
        case InputMouseButton::Middle:
            return "Middle";
        case InputMouseButton::Unknown:
        default:
            return "Unknown";
    }
}
}

InputSystem& InputSystem::getInstance() {
    static InputSystem instance;
    return instance;
}

void InputSystem::addListener(InputListener* listener) {
    if (listener == nullptr) {
        return;
    }

    if (std::find(listeners.begin(), listeners.end(), listener) == listeners.end()) {
        listeners.push_back(listener);
    }
}

void InputSystem::removeListener(InputListener* listener) {
    listeners.erase(std::remove(listeners.begin(), listeners.end(), listener), listeners.end());
}

void InputSystem::processKeyPressed(InputKey key) {
    std::ostringstream stream;
    stream << "Key pressed: " << keyToString(key);
    lastInputText = stream.str();

    for (InputListener* listener : listeners) {
        listener->onKeyPressed(key);
    }
}

void InputSystem::processKeyReleased(InputKey key) {
    std::ostringstream stream;
    stream << "Key released: " << keyToString(key);
    lastInputText = stream.str();

    for (InputListener* listener : listeners) {
        listener->onKeyReleased(key);
    }
}

void InputSystem::processMousePressed(InputMouseButton button, int x, int y) {
    std::ostringstream stream;
    stream << "Mouse pressed: " << mouseButtonToString(button)
           << " at " << x << ", " << y;
    lastInputText = stream.str();

    for (InputListener* listener : listeners) {
        listener->onMousePressed(button, x, y);
    }
}

void InputSystem::processMouseReleased(InputMouseButton button, int x, int y) {
    std::ostringstream stream;
    stream << "Mouse released: " << mouseButtonToString(button)
           << " at " << x << ", " << y;
    lastInputText = stream.str();

    for (InputListener* listener : listeners) {
        listener->onMouseReleased(button, x, y);
    }
}

const std::string& InputSystem::getLastInputText() const {
    return lastInputText;
}
