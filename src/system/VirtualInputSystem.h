#pragma once

#include "math/Vector2F.h"
#include "system/RawInputSystem.h"
#include "system/TouchControlSystem.h"

#include <string>
#include <unordered_map>
#include <vector>

enum class InputAction {
    Unknown,
    Move,
    Fire,
    Fire2,
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
    void bindKeyAxis2D(RawKey key, InputAction action, const Vector2F& value);
    void bindMouseButton(RawMouseButton button, InputAction action);
    void bindTouchControl(TouchControl control, InputAction action);
    void bindTouchControlAxis2D(TouchControl control, InputAction action, const Vector2F& value);
    bool loadBindingsFromFile(const std::string& path, std::string& errorMessage);

    void updateFromRawInput(const RawInputSystem& rawInputSystem, bool inputBlocked = false);
    void updateFromTouchControls(const TouchControlSystem& touchControlSystem, bool inputBlocked = false);

    Vector2F getAxis2D(InputAction action) const;
    bool isActionDown(InputAction action) const;
    bool wasActionPressed(InputAction action) const;
    bool wasActionReleased(InputAction action) const;

    void debugPrintState() const;

private:
    VirtualInputSystem() = default;

    struct KeyAxis2DBinding {
        RawKey key = RawKey::Unknown;
        InputAction action = InputAction::Unknown;
        Vector2F value = Vector2F::zero();
    };

    struct TouchAxis2DBinding {
        TouchControl control = TouchControl::Unknown;
        InputAction action = InputAction::Unknown;
        Vector2F value = Vector2F::zero();
    };

    std::unordered_map<RawKey, InputAction> keyBindings;
    std::unordered_map<RawMouseButton, InputAction> mouseButtonBindings;
    std::unordered_map<TouchControl, InputAction> touchControlBindings;
    std::vector<KeyAxis2DBinding> keyAxis2DBindings;
    std::vector<TouchAxis2DBinding> touchAxis2DBindings;

    std::unordered_map<InputAction, bool> currentActions;
    std::unordered_map<InputAction, bool> previousActions;
    std::unordered_map<InputAction, Vector2F> currentAxis2D;
    std::unordered_map<InputAction, Vector2F> previousAxis2D;
};

RawKey rawKeyFromString(const std::string& value);
RawMouseButton rawMouseButtonFromString(const std::string& value);
InputAction inputActionFromString(const std::string& value);
const char* inputActionToString(InputAction action);
