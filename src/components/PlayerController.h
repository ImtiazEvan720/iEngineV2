#pragma once

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
