#include "system/InputSystem.h"

#include <algorithm>
#include <sstream>

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

void InputSystem::processEvent(const sf::Event& event) {
    if (const auto* keyPressed = event.getIf<sf::Event::KeyPressed>()) {
        std::ostringstream stream;
        stream << "Key pressed: " << static_cast<int>(keyPressed->code);
        lastInputText = stream.str();

        for (InputListener* listener : listeners) {
            listener->onKeyPressed(keyPressed->code);
        }
    } else if (const auto* keyReleased = event.getIf<sf::Event::KeyReleased>()) {
        std::ostringstream stream;
        stream << "Key released: " << static_cast<int>(keyReleased->code);
        lastInputText = stream.str();

        for (InputListener* listener : listeners) {
            listener->onKeyReleased(keyReleased->code);
        }
    } else if (const auto* mousePressed = event.getIf<sf::Event::MouseButtonPressed>()) {
        std::ostringstream stream;
        stream << "Mouse pressed: " << static_cast<int>(mousePressed->button)
               << " at " << mousePressed->position.x << ", " << mousePressed->position.y;
        lastInputText = stream.str();

        for (InputListener* listener : listeners) {
            listener->onMousePressed(mousePressed->button, mousePressed->position.x, mousePressed->position.y);
        }
    } else if (const auto* mouseReleased = event.getIf<sf::Event::MouseButtonReleased>()) {
        std::ostringstream stream;
        stream << "Mouse released: " << static_cast<int>(mouseReleased->button)
               << " at " << mouseReleased->position.x << ", " << mouseReleased->position.y;
        lastInputText = stream.str();

        for (InputListener* listener : listeners) {
            listener->onMouseReleased(mouseReleased->button, mouseReleased->position.x, mouseReleased->position.y);
        }
    }
}

const std::string& InputSystem::getLastInputText() const {
    return lastInputText;
}
