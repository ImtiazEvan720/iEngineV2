#include "components/Component.h"

#include "Entity.h"

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

void Component::setEnabled(bool value) {

    if (value == enabled) {
        return;
    }

    this->enabled = value;

    if (!started) {
        return;
    }

    const bool entityEnabled = entity == nullptr || entity->isEnabled();
    onEnable(this->enabled && entityEnabled);
}

bool Component::isEnabled() const {
    return enabled;
}

void Component::onDestroy() {}

std::unique_ptr<Component> Component::clone() const {
    return nullptr;
}
