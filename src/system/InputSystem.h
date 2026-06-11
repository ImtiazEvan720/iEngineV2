#ifndef IENGINEV2_INPUTSYSTEM_H
#define IENGINEV2_INPUTSYSTEM_H

#include "system/InputListener.h"

#include <SFML/Window/Event.hpp>

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
    void processEvent(const sf::Event& event);

    const std::string& getLastInputText() const;

private:
    InputSystem() = default;
    ~InputSystem() = default;

    std::vector<InputListener*> listeners;
    std::string lastInputText = "No input yet";
};

#endif
