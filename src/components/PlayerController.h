#ifndef IENGINEV2_PLAYERCONTROLLER_H
#define IENGINEV2_PLAYERCONTROLLER_H

#include "components/Component.h"

class PlayerController : public Component {
public:
    void onStart() override;
    void onUpdate(float deltaTime) override;

private:
    void fire();

    class TransformComponent* transform = nullptr;
    class AnimationComponent* animator = nullptr;
    int firedBullets = 0;
};

#endif
