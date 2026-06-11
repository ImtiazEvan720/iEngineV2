#ifndef IENGINEV2_INPUTLISTENER_H
#define IENGINEV2_INPUTLISTENER_H

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>

class InputListener {
public:
    virtual ~InputListener() = default;

    virtual void onKeyPressed(sf::Keyboard::Key key);
    virtual void onKeyReleased(sf::Keyboard::Key key);
    virtual void onMousePressed(sf::Mouse::Button button, int x, int y);
    virtual void onMouseReleased(sf::Mouse::Button button, int x, int y);
};

#endif
