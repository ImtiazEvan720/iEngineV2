#ifndef IENGINEV2_PLAYERCONTROLLER_H
#define IENGINEV2_PLAYERCONTROLLER_H

#include "components/Component.h"
#include "system/InputListener.h"

class PlayerController : public Component, public InputListener {
public:
    void onKeyPressed(sf::Keyboard::Key key) override;
    void onKeyReleased(sf::Keyboard::Key key) override;
    void onMousePressed(sf::Mouse::Button button, int x, int y) override;
    void onMouseReleased(sf::Mouse::Button button, int x, int y) override;
    void onStart() override;
    void onUpdate(float deltaTime) override;

private:
    void fire();

    class TransformComponent* transform = nullptr;
    class AnimationComponent* animator = nullptr;
    int isMovingUp = false;
    int isMovingLeft = false;
    bool isMoving = false;
    int firedBullets = 0;
};

#endif
