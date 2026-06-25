#pragma once

#include "math/Vector2F.h"

#include <box2d/box2d.h>

#include <memory>

class CollisionComponent;

struct PhysicsRaycastHit {
    bool hit = false;
    Vector2F point = Vector2F::zero();
    Vector2F normal = Vector2F::zero();
    float fraction = 0.0f;
    int nodeVisits = 0;
    int leafVisits = 0;
    CollisionComponent* collider = nullptr;
};

class PhysicsSystem {
public:
    static PhysicsSystem& getInstance();

    PhysicsSystem(const PhysicsSystem& other) = delete;
    PhysicsSystem& operator=(const PhysicsSystem& other) = delete;
    PhysicsSystem(PhysicsSystem&& other) = delete;
    PhysicsSystem& operator=(PhysicsSystem&& other) = delete;
    ~PhysicsSystem();

    void initialize(float gravityX = 0.0f, float gravityY = 0.0f);
    void update(float deltaTime);
    void shutdown();

    b2WorldId getWorldId() const;
    bool isInitialized() const;
    PhysicsRaycastHit raycast(const Vector2F& start, const Vector2F& end) const;

private:
    PhysicsSystem() = default;

    b2WorldId worldId = b2_nullWorldId;
};
