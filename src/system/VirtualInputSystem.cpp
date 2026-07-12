#include "system/VirtualInputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>

namespace {
constexpr float AxisDeadZone = 0.001f;

bool getActionState(const std::unordered_map<InputAction, bool>& states, InputAction action) {
    const auto iterator = states.find(action);
    return iterator != states.end() && iterator->second;
}

Vector2F getAxisState(const std::unordered_map<InputAction, Vector2F>& states, InputAction action) {
    const auto iterator = states.find(action);
    if (iterator == states.end()) {
        return Vector2F::zero();
    }

    return iterator->second;
}

bool isAxisValueActive(const Vector2F& value) {
    return std::abs(value.x) > AxisDeadZone || std::abs(value.y) > AxisDeadZone;
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

void addAxis2DState(
    std::unordered_map<InputAction, Vector2F>& states,
    InputAction action,
    const Vector2F& value
) {
    if (action == InputAction::Unknown) {
        return;
    }

    auto iterator = states.find(action);
    if (iterator == states.end()) {
        states.emplace(action, value);
        return;
    }

    iterator->second += value;
}

void clampAxis2DStates(std::unordered_map<InputAction, Vector2F>& states) {
    for (auto& state : states) {
        state.second.x = std::clamp(state.second.x, -1.0f, 1.0f);
        state.second.y = std::clamp(state.second.y, -1.0f, 1.0f);
    }
}
}

VirtualInputSystem& VirtualInputSystem::getInstance() {
    static VirtualInputSystem instance;
    return instance;
}

void VirtualInputSystem::clearBindings() {
    keyBindings.clear();
    mouseButtonBindings.clear();
    touchControlBindings.clear();
    keyAxis2DBindings.clear();
    touchAxis2DBindings.clear();
}

void VirtualInputSystem::bindDefaultKeyboardMouse() {
    clearBindings();
    bindKeyAxis2D(RawKey::W, InputAction::Move, Vector2F(0.0f, -1.0f));
    bindKeyAxis2D(RawKey::S, InputAction::Move, Vector2F(0.0f, 1.0f));
    bindKeyAxis2D(RawKey::A, InputAction::Move, Vector2F(-1.0f, 0.0f));
    bindKeyAxis2D(RawKey::D, InputAction::Move, Vector2F(1.0f, 0.0f));
    bindKey(RawKey::Space, InputAction::Fire);
    bindKey(RawKey::Escape, InputAction::Fire2);
    bindMouseButton(RawMouseButton::Left, InputAction::Fire);
    bindTouchControlAxis2D(TouchControl::MoveStickUp, InputAction::Move, Vector2F(0.0f, -1.0f));
    bindTouchControlAxis2D(TouchControl::MoveStickDown, InputAction::Move, Vector2F(0.0f, 1.0f));
    bindTouchControlAxis2D(TouchControl::MoveStickLeft, InputAction::Move, Vector2F(-1.0f, 0.0f));
    bindTouchControlAxis2D(TouchControl::MoveStickRight, InputAction::Move, Vector2F(1.0f, 0.0f));
    bindTouchControl(TouchControl::FireButton, InputAction::Fire);
}

void VirtualInputSystem::bindKey(RawKey key, InputAction action) {
    if (key == RawKey::Unknown || action == InputAction::Unknown) {
        return;
    }

    keyBindings[key] = action;
}

void VirtualInputSystem::bindKeyAxis2D(RawKey key, InputAction action, const Vector2F& value) {
    if (key == RawKey::Unknown || action == InputAction::Unknown) {
        return;
    }

    keyAxis2DBindings.push_back({key, action, value});
}

void VirtualInputSystem::bindMouseButton(RawMouseButton button, InputAction action) {
    if (button == RawMouseButton::Unknown || action == InputAction::Unknown) {
        return;
    }

    mouseButtonBindings[button] = action;
}

void VirtualInputSystem::bindTouchControl(TouchControl control, InputAction action) {
    if (control == TouchControl::Unknown || action == InputAction::Unknown) {
        return;
    }

    touchControlBindings[control] = action;
}

void VirtualInputSystem::bindTouchControlAxis2D(
    TouchControl control,
    InputAction action,
    const Vector2F& value
) {
    if (control == TouchControl::Unknown || action == InputAction::Unknown) {
        return;
    }

    touchAxis2DBindings.push_back({control, action, value});
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

        const bool hasAxisX = binding->FindAttribute("axisX") != nullptr;
        const bool hasAxisY = binding->FindAttribute("axisY") != nullptr;
        const Vector2F axisValue(
            binding->FloatAttribute("axisX", 0.0f),
            binding->FloatAttribute("axisY", 0.0f)
        );
        const bool isAxisBinding = hasAxisX || hasAxisY;

        if (const char* keyText = binding->Attribute("key")) {
            const RawKey key = rawKeyFromString(keyText);
            if (key == RawKey::Unknown) {
                std::cerr << "Unknown raw key in binding file: " << keyText << std::endl;
            } else if (isAxisBinding) {
                bindKeyAxis2D(key, action, axisValue);
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

        if (const char* touchControlText = binding->Attribute("touchControl")) {
            const TouchControl control = touchControlFromString(touchControlText);
            if (control == TouchControl::Unknown) {
                std::cerr << "Unknown touch control in binding file: " << touchControlText << std::endl;
            } else if (isAxisBinding) {
                bindTouchControlAxis2D(control, action, axisValue);
            } else {
                bindTouchControl(control, action);
            }
        }
    }

    errorMessage.clear();
    return true;
}

void VirtualInputSystem::updateFromRawInput(const RawInputSystem& rawInputSystem, bool inputBlocked) {
    previousActions = currentActions;
    previousAxis2D = currentAxis2D;
    currentActions.clear();
    currentAxis2D.clear();

    if (inputBlocked) {
        return;
    }

    for (const auto& binding : keyBindings) {
        setActionState(currentActions, binding.second, rawInputSystem.isKeyDown(binding.first));
    }

    for (const auto& binding : mouseButtonBindings) {
        setActionState(currentActions, binding.second, rawInputSystem.isMouseButtonDown(binding.first));
    }

    for (const KeyAxis2DBinding& binding : keyAxis2DBindings) {
        if (rawInputSystem.isKeyDown(binding.key)) {
            addAxis2DState(currentAxis2D, binding.action, binding.value);
        }
    }

    clampAxis2DStates(currentAxis2D);
}

void VirtualInputSystem::updateFromTouchControls(
    const TouchControlSystem& touchControlSystem,
    bool inputBlocked
) {
    if (inputBlocked || !touchControlSystem.isEnabled()) {
        return;
    }

    for (const auto& binding : touchControlBindings) {
        setActionState(currentActions, binding.second, touchControlSystem.isControlDown(binding.first));
    }

    for (const TouchAxis2DBinding& binding : touchAxis2DBindings) {
        if (touchControlSystem.isControlDown(binding.control)) {
            addAxis2DState(currentAxis2D, binding.action, binding.value);
        }
    }

    clampAxis2DStates(currentAxis2D);
}

Vector2F VirtualInputSystem::getAxis2D(InputAction action) const {
    return getAxisState(currentAxis2D, action);
}

bool VirtualInputSystem::isActionDown(InputAction action) const {
    return getActionState(currentActions, action) || isAxisValueActive(getAxis2D(action));
}

bool VirtualInputSystem::wasActionPressed(InputAction action) const {
    const bool current = getActionState(currentActions, action)
        || isAxisValueActive(getAxisState(currentAxis2D, action));
    const bool previous = getActionState(previousActions, action)
        || isAxisValueActive(getAxisState(previousAxis2D, action));
    return current && !previous;
}

bool VirtualInputSystem::wasActionReleased(InputAction action) const {
    const bool current = getActionState(currentActions, action)
        || isAxisValueActive(getAxisState(currentAxis2D, action));
    const bool previous = getActionState(previousActions, action)
        || isAxisValueActive(getAxisState(previousAxis2D, action));
    return !current && previous;
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

    for (const auto& axisState : currentAxis2D) {
        std::cout << "  " << inputActionToString(axisState.first)
                  << " axis=(" << axisState.second.x << ", " << axisState.second.y << ")"
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

    if (normalized == "move") {
        return InputAction::Move;
    }

    if (normalized == "fire") {
        return InputAction::Fire;
    }

    if (normalized == "fire2") {
        return InputAction::Fire2;
    }

    if (normalized == "pause") {
        return InputAction::Pause;
    }

    return InputAction::Unknown;
}

const char* inputActionToString(InputAction action) {
    switch (action) {
        case InputAction::Move:
            return "Move";
        case InputAction::Fire:
            return "Fire";
        case InputAction::Fire2:
            return "Fire2";
        case InputAction::Pause:
            return "Pause";
        case InputAction::Unknown:
        default:
            return "Unknown";
    }
}
