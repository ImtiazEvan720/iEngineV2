#include "system/VirtualInputSystem.h"

#include "tinyxml2.h"

#include <cctype>
#include <iostream>

namespace {
bool getActionState(const std::unordered_map<InputAction, bool>& states, InputAction action) {
    const auto iterator = states.find(action);
    return iterator != states.end() && iterator->second;
}

std::string normalizeInputName(std::string value) {
    for (char& character : value) {
        if (character == '_' || character == '-' || character == ' ') {
            character = '\0';
        }
    }

    std::string normalized;
    normalized.reserve(value.size());
    for (char character : value) {
        if (character != '\0') {
            normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
        }
    }

    return normalized;
}

void setActionState(std::unordered_map<InputAction, bool>& states, InputAction action, bool down) {
    if (action == InputAction::Unknown) {
        return;
    }

    states[action] = states[action] || down;
}
}

VirtualInputSystem& VirtualInputSystem::getInstance() {
    static VirtualInputSystem instance;
    return instance;
}

void VirtualInputSystem::clearBindings() {
    keyBindings.clear();
    mouseButtonBindings.clear();
}

void VirtualInputSystem::bindDefaultKeyboardMouse() {
    clearBindings();
    bindKey(RawKey::W, InputAction::MoveUp);
    bindKey(RawKey::S, InputAction::MoveDown);
    bindKey(RawKey::A, InputAction::MoveLeft);
    bindKey(RawKey::D, InputAction::MoveRight);
    bindKey(RawKey::Space, InputAction::Fire);
    bindMouseButton(RawMouseButton::Left, InputAction::Fire);
}

void VirtualInputSystem::bindKey(RawKey key, InputAction action) {
    if (key == RawKey::Unknown || action == InputAction::Unknown) {
        return;
    }

    keyBindings[key] = action;
}

void VirtualInputSystem::bindMouseButton(RawMouseButton button, InputAction action) {
    if (button == RawMouseButton::Unknown || action == InputAction::Unknown) {
        return;
    }

    mouseButtonBindings[button] = action;
}

bool VirtualInputSystem::loadBindingsFromFile(const std::string& path, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load input bindings: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* inputElement = document.FirstChildElement("input");
    if (inputElement == nullptr) {
        errorMessage = "Input binding file is missing input root.";
        return false;
    }

    const tinyxml2::XMLElement* bindingsElement = inputElement->FirstChildElement("bindings");
    if (bindingsElement == nullptr) {
        errorMessage = "Input binding file is missing bindings element.";
        return false;
    }

    clearBindings();

    for (const tinyxml2::XMLElement* binding = bindingsElement->FirstChildElement("binding");
         binding != nullptr;
         binding = binding->NextSiblingElement("binding")) {
        const char* actionText = binding->Attribute("action");
        if (actionText == nullptr) {
            continue;
        }

        const InputAction action = inputActionFromString(actionText);
        if (action == InputAction::Unknown) {
            std::cerr << "Unknown input action in binding file: " << actionText << std::endl;
            continue;
        }

        if (const char* keyText = binding->Attribute("key")) {
            const RawKey key = rawKeyFromString(keyText);
            if (key == RawKey::Unknown) {
                std::cerr << "Unknown raw key in binding file: " << keyText << std::endl;
            } else {
                bindKey(key, action);
            }
        }

        if (const char* mouseText = binding->Attribute("mouse")) {
            const RawMouseButton button = rawMouseButtonFromString(mouseText);
            if (button == RawMouseButton::Unknown) {
                std::cerr << "Unknown raw mouse button in binding file: " << mouseText << std::endl;
            } else {
                bindMouseButton(button, action);
            }
        }
    }

    errorMessage.clear();
    return true;
}

void VirtualInputSystem::updateFromRawInput(const RawInputSystem& rawInputSystem) {
    previousActions = currentActions;
    currentActions.clear();

    for (const auto& binding : keyBindings) {
        setActionState(currentActions, binding.second, rawInputSystem.isKeyDown(binding.first));
    }

    for (const auto& binding : mouseButtonBindings) {
        setActionState(currentActions, binding.second, rawInputSystem.isMouseButtonDown(binding.first));
    }
}

bool VirtualInputSystem::isActionDown(InputAction action) const {
    return getActionState(currentActions, action);
}

bool VirtualInputSystem::wasActionPressed(InputAction action) const {
    return getActionState(currentActions, action) && !getActionState(previousActions, action);
}

bool VirtualInputSystem::wasActionReleased(InputAction action) const {
    return !getActionState(currentActions, action) && getActionState(previousActions, action);
}

void VirtualInputSystem::debugPrintState() const {
    std::cout << "VirtualInputSystem state" << std::endl;

    for (const auto& actionState : currentActions) {
        std::cout << "  " << inputActionToString(actionState.first)
                  << " down=" << (actionState.second ? "true" : "false")
                  << " pressed=" << (wasActionPressed(actionState.first) ? "true" : "false")
                  << " released=" << (wasActionReleased(actionState.first) ? "true" : "false")
                  << std::endl;
    }
}

RawKey rawKeyFromString(const std::string& value) {
    const std::string normalized = normalizeInputName(value);

    if (normalized == "w") {
        return RawKey::W;
    }

    if (normalized == "a") {
        return RawKey::A;
    }

    if (normalized == "s") {
        return RawKey::S;
    }

    if (normalized == "d") {
        return RawKey::D;
    }

    if (normalized == "space") {
        return RawKey::Space;
    }

    if (normalized == "escape" || normalized == "esc") {
        return RawKey::Escape;
    }

    return RawKey::Unknown;
}

RawMouseButton rawMouseButtonFromString(const std::string& value) {
    const std::string normalized = normalizeInputName(value);

    if (normalized == "left") {
        return RawMouseButton::Left;
    }

    if (normalized == "right") {
        return RawMouseButton::Right;
    }

    if (normalized == "middle") {
        return RawMouseButton::Middle;
    }

    return RawMouseButton::Unknown;
}

InputAction inputActionFromString(const std::string& value) {
    const std::string normalized = normalizeInputName(value);

    if (normalized == "moveup") {
        return InputAction::MoveUp;
    }

    if (normalized == "movedown") {
        return InputAction::MoveDown;
    }

    if (normalized == "moveleft") {
        return InputAction::MoveLeft;
    }

    if (normalized == "moveright") {
        return InputAction::MoveRight;
    }

    if (normalized == "fire") {
        return InputAction::Fire;
    }

    if (normalized == "pause") {
        return InputAction::Pause;
    }

    return InputAction::Unknown;
}

const char* inputActionToString(InputAction action) {
    switch (action) {
        case InputAction::MoveUp:
            return "MoveUp";
        case InputAction::MoveDown:
            return "MoveDown";
        case InputAction::MoveLeft:
            return "MoveLeft";
        case InputAction::MoveRight:
            return "MoveRight";
        case InputAction::Fire:
            return "Fire";
        case InputAction::Pause:
            return "Pause";
        case InputAction::Unknown:
        default:
            return "Unknown";
    }
}
