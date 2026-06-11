#include "Entity.h"

#include "system/InputListener.h"
#include "system/InputSystem.h"

int Entity::globalId = 0;

Entity::Entity()
    : name("Entity"), tag("Default"), id(globalId++) {}

Entity::~Entity() {
    destroy();
}

Entity::Entity(Entity&& other) noexcept
    : components(std::move(other.components)),
      name(std::move(other.name)),
      tag(std::move(other.tag)),
      id(other.id),
      destroyed(other.destroyed),
      updating(false) {
    refreshComponentOwners();
}

Entity& Entity::operator=(Entity&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    destroy();

    components = std::move(other.components);
    name = std::move(other.name);
    tag = std::move(other.tag);
    id = other.id;
    destroyed = other.destroyed;
    updating = false;
    refreshComponentOwners();

    return *this;
}

void Entity::refreshComponentOwners() {
    for (const auto& component : components) {
        component->setEntity(this);
    }
}

void Entity::update(float deltaTime) {
    if (destroyed) {
        return;
    }

    updating = true;

    for (const auto& component : components) {
        component->onUpdate(deltaTime);

        if (destroyed) {
            break;
        }
    }

    updating = false;

    if (destroyed) {
        destroyComponents();
    }
}

void Entity::destroy() {
    if (destroyed) {
        return;
    }

    destroyed = true;

    if (updating) {
        return;
    }

    destroyComponents();
}

void Entity::destroyComponents() {
    for (const auto& component : components) {
        if (auto* listener = dynamic_cast<InputListener*>(component.get())) {
            InputSystem::getInstance().removeListener(listener);
        }

        component->onDestroy();
    }

    components.clear();
}

bool Entity::isDestroyed() const {
    return destroyed;
}

void Entity::addInputListeners(InputSystem& inputSystem) {
    for (const auto& component : components) {
        if (auto* listener = dynamic_cast<InputListener*>(component.get())) {
            inputSystem.addListener(listener);
        }
    }
}

void Entity::removeInputListeners(InputSystem& inputSystem) {
    for (const auto& component : components) {
        if (auto* listener = dynamic_cast<InputListener*>(component.get())) {
            inputSystem.removeListener(listener);
        }
    }
}

int Entity::getId() const {
    return id;
}

const std::string& Entity::getName() const {
    return name;
}

const std::string& Entity::getTag() const {
    return tag;
}

void Entity::setName(const std::string& name) {
    this->name = name;
}

void Entity::setTag(const std::string& tag) {
    this->tag = tag;
}
