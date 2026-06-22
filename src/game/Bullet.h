#pragma once

#include "components/CollisionComponent.h"
#include "components/Component.h"

class Bullet : public Component, public CollisionListener {
public:
    Bullet() = default;
    ~Bullet() override = default;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onEnable(bool value) override;
    void onDestroy() override;
    void onCollisionEnter(CollisionComponent& self, CollisionComponent& other) override;

    float speed = 300.0f;

    private:
        class TransformComponent* transform = nullptr;
        CollisionComponent* collision = nullptr;

};
