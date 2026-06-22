#pragma once

#include "system/InputListener.h"
#include "system/InputTypes.h"

#include <string>
#include <vector>

class InputSystem {
public:
    static InputSystem& getInstance();

    InputSystem(const InputSystem& other) = delete;
    InputSystem& operator=(const InputSystem& other) = delete;
    InputSystem(InputSystem&& other) = delete;
    InputSystem& operator=(InputSystem&& other) = delete;

    void addListener(InputListener* listener);
    void removeListener(InputListener* listener);
    void processKeyPressed(InputKey key);
    void processKeyReleased(InputKey key);
    void processMousePressed(InputMouseButton button, int x, int y);
    void processMouseReleased(InputMouseButton button, int x, int y);

    const std::string& getLastInputText() const;

private:
    InputSystem() = default;
    ~InputSystem() = default;

    std::vector<InputListener*> listeners;
    std::string lastInputText = "No input yet";
};
