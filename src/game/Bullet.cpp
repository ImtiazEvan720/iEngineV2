#include "game/Bullet.h"
#include "misc/Level.h"
#include "components/CollisionComponent.h"
#include "components/TransformComponent.h"
#include "Entity.h"
#include <iostream>
#include <math/Vector2F.h>

void Bullet::onStart() {

    std::cout << "Bullet created. with entity id: " << getEntity()->getId() << std::endl;
    transform = getEntity()->getComponent<TransformComponent>();
    if (transform == nullptr) {
        std::cerr << "Bullet requires a TransformComponent to function properly." << std::endl;
    }

    collision = getEntity()->getComponent<CollisionComponent>();
    if (collision == nullptr) {
        std::cerr << "Bullet requires a CollisionComponent to listen for collisions." << std::endl;
        return;
    }

    collision->setListener(this);
}

void Bullet::onUpdate(float deltaTime) {
    if (transform != nullptr) {
        Vector2F position = transform->getPosition();
        const float distance = speed * deltaTime;
        const float rotation = transform->getRotation();

        if (rotation == 0.0f) {
            position.y -= distance;
        } else if (rotation == 90.0f) {
            position.x += distance;
        } else if (rotation == 180.0f) {
            position.y += distance;
        } else if (rotation == -90.0f) {
            position.x -= distance;
        }

        transform->setPosition(position);
        if (collision != nullptr) {
            collision->syncBodyToTransform();
        }
    }   
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
    Level::getCurrentLevel().destroyEntity(getEntity());
}
