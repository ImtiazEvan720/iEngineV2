#pragma once

#include "components/Component.h"

class PlayerController : public Component {
public:
    void onStart() override;
    void onUpdate(float deltaTime) override;
    std::unique_ptr<Component> clone() const override;

private:
    void fire();

    class TransformComponent* transform = nullptr;
    class AnimationComponent* animator = nullptr;
    int firedBullets = 0;
};
