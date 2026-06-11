#ifndef IENGINEV2_BRICK_H
#define IENGINEV2_BRICK_H

#include "components/CollisionComponent.h"
#include "components/Component.h"

class Brick : public Component, public CollisionListener {
public:
    Brick() = default;
    ~Brick() override = default;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;
    void onCollisionEnter(CollisionComponent& self, CollisionComponent& other) override;

private:
    CollisionComponent* collision = nullptr;
    class AnimationComponent* destroyAnimation = nullptr;
    bool destroying = false;
};

#endif
