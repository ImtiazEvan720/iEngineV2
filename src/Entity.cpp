#include "Entity.h"

#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/PrefabAsset.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/InputListener.h"
#include "system/InputSystem.h"

#include <algorithm>
#include <iostream>

int Entity::globalId = 0;

Entity::Entity()
    : name("Entity"), tag("Default"), id(globalId++) {}

Entity::~Entity() {
    destroy();
}

Entity::Entity(Entity&& other) noexcept
    : components(std::move(other.components)),
      parentEntity(other.parentEntity),
      children(std::move(other.children)),
      name(std::move(other.name)),
      tag(std::move(other.tag)),
      id(other.id),
      enabled(other.enabled),
      destroyed(other.destroyed),
      updating(false) {
    refreshComponentOwners();
    rebindParentLinksFrom(&other);
    other.parentEntity = nullptr;
    other.children.clear();
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
    parentEntity = other.parentEntity;
    children = std::move(other.children);
    enabled = other.enabled;
    destroyed = other.destroyed;
    updating = false;
    refreshComponentOwners();
    rebindParentLinksFrom(&other);
    other.parentEntity = nullptr;
    other.children.clear();

    return *this;
}

void Entity::refreshComponentOwners() {
    for (const auto& component : components) {
        component->setEntity(this);
    }

    syncTransformParent();
}

void Entity::update(float deltaTime) {
    if (destroyed || !enabled) {
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

    std::vector<Entity*> childSnapshot = children;
    for (Entity* child : childSnapshot) {
        if (child != nullptr && child->parentEntity == this) {
            child->clearParent();
        }
    }
    children.clear();
    clearParent();

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
    if (!enabled) {
        return;
    }

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

bool Entity::isEnabled() const {
    return enabled;
}

Entity* Entity::getParent() {
    return parentEntity;
}

const Entity* Entity::getParent() const {
    return parentEntity;
}

const std::vector<Entity*>& Entity::getChildren() const {
    return children;
}

bool Entity::isChildOf(const Entity& possibleParent) const {
    const Entity* current = parentEntity;
    while (current != nullptr) {
        if (current == &possibleParent) {
            return true;
        }

        current = current->parentEntity;
    }

    return false;
}

void Entity::setEnabled(bool enabled) {
    if (this->enabled == enabled) {
        return;
    }

    this->enabled = enabled;

    if (!this->enabled) {
        removeInputListeners(InputSystem::getInstance());
    } else {
        addInputListeners(InputSystem::getInstance());
    }

    for (const auto& component : components) {
        component->onEnable(this->enabled);
    }
}

bool Entity::setParent(Entity* parent, bool keepWorldTransform) {
    if (parent == this) {
        return false;
    }

    if (parent != nullptr && parent->isChildOf(*this)) {
        return false;
    }

    if (parentEntity == parent) {
        return true;
    }

    TransformComponent* transform = getComponent<TransformComponent>();
    Vector2F worldPosition = Vector2F::zero();
    float worldRotation = 0.0f;
    if (transform != nullptr && keepWorldTransform) {
        worldPosition = transform->getWorldPosition();
        worldRotation = transform->getWorldRotation();
    }

    if (parentEntity != nullptr) {
        parentEntity->removeChildReference(this);
    }

    parentEntity = parent;

    if (parentEntity != nullptr
        && std::find(
            parentEntity->children.begin(),
            parentEntity->children.end(),
            this
        ) == parentEntity->children.end()) {
        parentEntity->children.push_back(this);
    }

    syncTransformParent();

    if (transform != nullptr && keepWorldTransform) {
        TransformComponent* parentTransform =
            parentEntity == nullptr ? nullptr : parentEntity->getComponent<TransformComponent>();

        if (parentTransform != nullptr) {
            const Vector2F parentWorldPosition = parentTransform->getWorldPosition();
            transform->setPosition(Vector2F(
                worldPosition.x - parentWorldPosition.x,
                worldPosition.y - parentWorldPosition.y
            ));
            transform->setRotation(worldRotation - parentTransform->getWorldRotation());
        } else {
            transform->setPosition(worldPosition);
            transform->setRotation(worldRotation);
        }
    }

    syncChildTransformParents();
    return true;
}

void Entity::clearParent(bool keepWorldTransform) {
    (void)setParent(nullptr, keepWorldTransform);
}

void Entity::setName(const std::string& name) {
    this->name = name;
}

void Entity::setTag(const std::string& tag) {
    this->tag = tag;
}

void Entity::removeChildReference(Entity* child) {
    children.erase(
        std::remove(children.begin(), children.end(), child),
        children.end()
    );
}

void Entity::syncTransformParent() {
    TransformComponent* transform = getComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    TransformComponent* parentTransform =
        parentEntity == nullptr ? nullptr : parentEntity->getComponent<TransformComponent>();
    transform->setParent(parentTransform);
}

void Entity::syncChildTransformParents() {
    for (Entity* child : children) {
        if (child != nullptr) {
            child->syncTransformParent();
        }
    }
}

void Entity::rebindParentLinksFrom(Entity* oldAddress) {
    if (oldAddress == nullptr) {
        return;
    }

    if (parentEntity != nullptr) {
        for (Entity*& child : parentEntity->children) {
            if (child == oldAddress) {
                child = this;
            }
        }
    }

    for (Entity* child : children) {
        if (child != nullptr && child->parentEntity == oldAddress) {
            child->parentEntity = this;
            child->syncTransformParent();
        }
    }

    syncTransformParent();
}

Entity* Entity::spawnPrefab(const std::string& prefabName, const Vector2F& position, float rotation) {
    PrefabAsset* prefabAsset = AssetManager::getInstance().getPrefabAssetByName(prefabName);
    if (prefabAsset == nullptr || !prefabAsset->isLoaded()) {
        std::cerr << "Entity::spawnPrefab failed. Prefab asset is not loaded: "
                  << prefabName << std::endl;
        return nullptr;
    }

    std::string errorMessage;

    Entity* entity = PrefabSerializer::instantiate(
        prefabAsset->getPath(),
        Level::getCurrentLevel(),
        position,
        errorMessage
    );

    if (entity == nullptr) {
        std::cerr << "Entity::spawnPrefab failed for '" << prefabAsset->getPath() << "': "
                  << (errorMessage.empty() ? "unknown error" : errorMessage) << std::endl;
        return nullptr;
    }

    if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
        transform->setRotation(rotation);
    }

    return entity;
}

Entity* Entity::spawnPrefab(const std::string& prefabName, float x, float y, float rotation) {
    return spawnPrefab(prefabName, Vector2F(x, y), rotation);
}
