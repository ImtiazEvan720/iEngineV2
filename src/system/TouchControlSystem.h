#ifndef IENGINEV2_TOUCHCONTROLSYSTEM_H
#define IENGINEV2_TOUCHCONTROLSYSTEM_H

#include "system/RawInputSystem.h"

#include <string>
#include <unordered_map>

enum class TouchControl {
    Unknown,
    MoveStickUp,
    MoveStickDown,
    MoveStickLeft,
    MoveStickRight,
    FireButton
};

class TouchControlSystem {
public:
    static TouchControlSystem& getInstance();

    TouchControlSystem(const TouchControlSystem& other) = delete;
    TouchControlSystem& operator=(const TouchControlSystem& other) = delete;
    TouchControlSystem(TouchControlSystem&& other) = delete;
    TouchControlSystem& operator=(TouchControlSystem&& other) = delete;

    void setEnabled(bool value);
    bool isEnabled() const;

    void updateFromRawInput(const RawInputSystem& rawInputSystem, float screenWidth, float screenHeight);

    bool isControlDown(TouchControl control) const;
    bool wasControlPressed(TouchControl control) const;
    bool wasControlReleased(TouchControl control) const;

    void debugPrintState() const;

private:
    TouchControlSystem() = default;

    void setControlState(TouchControl control, bool down);
    const RawTouch* findActiveMoveTouch(const RawInputSystem& rawInputSystem) const;

    bool enabled = false;
    int activeMoveTouchId = -1;
    float moveStartX = 0.0f;
    float moveStartY = 0.0f;
    float joystickDeadzone = 24.0f;

    std::unordered_map<TouchControl, bool> currentControls;
    std::unordered_map<TouchControl, bool> previousControls;
};

TouchControl touchControlFromString(const std::string& value);
const char* touchControlToString(TouchControl control);

#endif
