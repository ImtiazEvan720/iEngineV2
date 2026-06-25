#include "game/Bullet.h"
#include "components/CollisionComponent.h"
#include "system/ScriptSystem.h"
#include "Entity.h"
#include <iostream>

void Bullet::onStart() {

    std::cout << "Bullet created. with entity id: " << getEntity()->getId() << std::endl;
    collision = getEntity()->getComponent<CollisionComponent>();
    if (collision == nullptr) {
        std::cerr << "Bullet requires a CollisionComponent to listen for collisions." << std::endl;
        return;
    }

    collision->setListener(this);
}

void Bullet::onUpdate(float deltaTime) {
    (void)deltaTime;
}

void Bullet::onEnable(bool value) {
    (void)value;
}

void Bullet::onDestroy() {
    if (collision != nullptr) {
        collision->setListener(nullptr);
        collision = nullptr;
    }
}

void Bullet::onCollisionEnter(CollisionComponent& self, CollisionComponent& other) {
    std::cout << "Bullet collision: " << self.getName()
              << " touched " << other.getName() << std::endl;

    Entity* entity = getEntity();
    if (entity == nullptr) {
        return;
    }

    if (!ScriptSystem::getInstance().callEntityCollisionFunction(*entity, "onCollisionEnter", self, other)) {
        entity->setEnabled(false);
    }
}
