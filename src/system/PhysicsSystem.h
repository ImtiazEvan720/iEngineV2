#pragma once

#include <box2d/box2d.h>

#include <memory>

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

private:
    PhysicsSystem() = default;

    b2WorldId worldId = b2_nullWorldId;
};
