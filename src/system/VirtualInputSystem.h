#pragma once

#include "system/RawInputSystem.h"
#include "system/TouchControlSystem.h"

#include <string>
#include <unordered_map>

enum class InputAction {
    Unknown,
    MoveUp,
    MoveDown,
    MoveLeft,
    MoveRight,
    Fire,
    Pause
};

class VirtualInputSystem {
public:
    static VirtualInputSystem& getInstance();

    VirtualInputSystem(const VirtualInputSystem& other) = delete;
    VirtualInputSystem& operator=(const VirtualInputSystem& other) = delete;
    VirtualInputSystem(VirtualInputSystem&& other) = delete;
    VirtualInputSystem& operator=(VirtualInputSystem&& other) = delete;

    void clearBindings();
    void bindDefaultKeyboardMouse();
    void bindKey(RawKey key, InputAction action);
    void bindMouseButton(RawMouseButton button, InputAction action);
    void bindTouchControl(TouchControl control, InputAction action);
    bool loadBindingsFromFile(const std::string& path, std::string& errorMessage);

    void updateFromRawInput(const RawInputSystem& rawInputSystem);
    void updateFromTouchControls(const TouchControlSystem& touchControlSystem);

    bool isActionDown(InputAction action) const;
    bool wasActionPressed(InputAction action) const;
    bool wasActionReleased(InputAction action) const;

    void debugPrintState() const;

private:
    VirtualInputSystem() = default;

    std::unordered_map<RawKey, InputAction> keyBindings;
    std::unordered_map<RawMouseButton, InputAction> mouseButtonBindings;
    std::unordered_map<TouchControl, InputAction> touchControlBindings;

    std::unordered_map<InputAction, bool> currentActions;
    std::unordered_map<InputAction, bool> previousActions;
};

RawKey rawKeyFromString(const std::string& value);
RawMouseButton rawMouseButtonFromString(const std::string& value);
InputAction inputActionFromString(const std::string& value);
const char* inputActionToString(InputAction action);
