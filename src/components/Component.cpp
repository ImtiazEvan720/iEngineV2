#include "components/Component.h"

Entity* Component::getEntity() {
    return entity;
}

const Entity* Component::getEntity() const {
    return entity;
}

void Component::setEntity(Entity* entity) {
    this->entity = entity;
}

void Component::onStart() {}

void Component::onUpdate(float deltaTime) {
    (void)deltaTime;
}

void Component::onEnable(bool value) {
    (void)value;
}

void Component::onDestroy() {}
