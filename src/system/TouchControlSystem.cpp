#include "system/TouchControlSystem.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

namespace {
bool getControlState(const std::unordered_map<TouchControl, bool>& states, TouchControl control) {
    const auto iterator = states.find(control);
    return iterator != states.end() && iterator->second;
}

std::string normalizeTouchControlName(std::string value) {
    std::string normalized;
    normalized.reserve(value.size());

    for (char character : value) {
        if (character == '_' || character == '-' || character == ' ') {
            continue;
        }

        normalized.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(character))));
    }

    return normalized;
}
}

TouchControlSystem& TouchControlSystem::getInstance() {
    static TouchControlSystem instance;
    return instance;
}

void TouchControlSystem::setEnabled(bool value) {
    enabled = value;
    if (!enabled) {
        activeMoveTouchId = -1;
        currentControls.clear();
        previousControls.clear();
    }
}

bool TouchControlSystem::isEnabled() const {
    return enabled;
}

void TouchControlSystem::updateFromRawInput(
    const RawInputSystem& rawInputSystem,
    float screenWidth,
    float screenHeight
) {
    previousControls = currentControls;
    currentControls.clear();

    if (!enabled || screenWidth <= 0.0f || screenHeight <= 0.0f) {
        activeMoveTouchId = -1;
        return;
    }

    const RawTouch* moveTouch = findActiveMoveTouch(rawInputSystem);
    if (moveTouch == nullptr) {
        activeMoveTouchId = -1;

        for (const RawTouch& touch : rawInputSystem.getTouches()) {
            if (touch.down && touch.x < screenWidth * 0.5f) {
                activeMoveTouchId = touch.id;
                moveStartX = touch.x;
                moveStartY = touch.y;
                moveTouch = &touch;
                break;
            }
        }
    }

    if (moveTouch != nullptr) {
        const float deadzone = std::max(joystickDeadzone, std::min(screenWidth, screenHeight) * 0.04f);
        const float deltaX = moveTouch->x - moveStartX;
        const float deltaY = moveTouch->y - moveStartY;

        setControlState(TouchControl::MoveStickUp, deltaY < -deadzone);
        setControlState(TouchControl::MoveStickDown, deltaY > deadzone);
        setControlState(TouchControl::MoveStickLeft, deltaX < -deadzone);
        setControlState(TouchControl::MoveStickRight, deltaX > deadzone);
    }

    for (const RawTouch& touch : rawInputSystem.getTouches()) {
        if (touch.down && touch.x >= screenWidth * 0.5f) {
            setControlState(TouchControl::FireButton, true);
            break;
        }
    }
}

bool TouchControlSystem::isControlDown(TouchControl control) const {
    return getControlState(currentControls, control);
}

bool TouchControlSystem::wasControlPressed(TouchControl control) const {
    return getControlState(currentControls, control) && !getControlState(previousControls, control);
}

bool TouchControlSystem::wasControlReleased(TouchControl control) const {
    return !getControlState(currentControls, control) && getControlState(previousControls, control);
}

void TouchControlSystem::debugPrintState() const {
    std::cout << "TouchControlSystem state enabled="
              << (enabled ? "true" : "false") << std::endl;

    for (const auto& controlState : currentControls) {
        std::cout << "  " << touchControlToString(controlState.first)
                  << " down=" << (controlState.second ? "true" : "false")
                  << " pressed=" << (wasControlPressed(controlState.first) ? "true" : "false")
                  << " released=" << (wasControlReleased(controlState.first) ? "true" : "false")
                  << std::endl;
    }
}

void TouchControlSystem::setControlState(TouchControl control, bool down) {
    if (control == TouchControl::Unknown) {
        return;
    }

    currentControls[control] = currentControls[control] || down;
}

const RawTouch* TouchControlSystem::findActiveMoveTouch(const RawInputSystem& rawInputSystem) const {
    if (activeMoveTouchId < 0) {
        return nullptr;
    }

    for (const RawTouch& touch : rawInputSystem.getTouches()) {
        if (touch.id == activeMoveTouchId && touch.down) {
            return &touch;
        }
    }

    return nullptr;
}

TouchControl touchControlFromString(const std::string& value) {
    const std::string normalized = normalizeTouchControlName(value);

    if (normalized == "movestickup") {
        return TouchControl::MoveStickUp;
    }

    if (normalized == "movestickdown") {
        return TouchControl::MoveStickDown;
    }

    if (normalized == "movestickleft") {
        return TouchControl::MoveStickLeft;
    }

    if (normalized == "movestickright") {
        return TouchControl::MoveStickRight;
    }

    if (normalized == "firebutton") {
        return TouchControl::FireButton;
    }

    return TouchControl::Unknown;
}

const char* touchControlToString(TouchControl control) {
    switch (control) {
        case TouchControl::MoveStickUp:
            return "MoveStickUp";
        case TouchControl::MoveStickDown:
            return "MoveStickDown";
        case TouchControl::MoveStickLeft:
            return "MoveStickLeft";
        case TouchControl::MoveStickRight:
            return "MoveStickRight";
        case TouchControl::FireButton:
            return "FireButton";
        case TouchControl::Unknown:
        default:
            return "Unknown";
    }
}
