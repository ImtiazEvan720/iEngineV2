#include "Entity.h"

#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Guid.h"
#include "misc/Level.h"
#include "misc/PrefabAsset.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/EngineState.h"
#include "system/InputListener.h"
#include "system/InputSystem.h"

#include <algorithm>
#include <iostream>

int Entity::globalId = 0;

Entity::Entity()
    : name("Entity"),
      tag("Default"),
      guid(Guid::generate()),
      id(globalId++),
      displayOrder(id) {}

Entity::~Entity() {
    destroy();
}

Entity::Entity(Entity&& other) noexcept
    : components(std::move(other.components)),
      parentEntity(other.parentEntity),
      children(std::move(other.children)),
      name(std::move(other.name)),
      tag(std::move(other.tag)),
      guid(std::move(other.guid)),
      id(other.id),
      displayOrder(other.displayOrder),
      enabled(other.enabled),
      destroyed(other.destroyed),
      persistent(other.persistent),
      componentStartupDeferred(other.componentStartupDeferred),
      hasDeferredComponentStartup(other.hasDeferredComponentStartup),
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
    guid = std::move(other.guid);
    id = other.id;
    displayOrder = other.displayOrder;
    parentEntity = other.parentEntity;
    children = std::move(other.children);
    enabled = other.enabled;
    destroyed = other.destroyed;
    persistent = other.persistent;
    componentStartupDeferred = other.componentStartupDeferred;
    hasDeferredComponentStartup = other.hasDeferredComponentStartup;
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

bool Entity::shouldStartComponentsImmediately() const {
    return !componentStartupDeferred && EngineState::getInstance().isPlaying();
}

void Entity::startComponent(Component& component) {
    if (component.started) {
        return;
    }

    component.started = true;
    component.onStart();

    if (!enabled) {
        component.onEnable(false);
    }
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

void Entity::destroy(bool destroyChildren) {
    if (destroyed) {
        return;
    }

    std::vector<Entity*> childSnapshot = children;
    for (Entity* child : childSnapshot) {
        if (child != nullptr && child->parentEntity == this) {
            if (destroyChildren) {
                child->destroy(destroyChildren);
            } else {
                child->clearParent();
            }
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

        if (component->started) {
            component->onDestroy();
            component->started = false;
        }
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

void Entity::setComponentStartupDeferred(bool deferred) {
    componentStartupDeferred = deferred;
}

void Entity::startDeferredComponents() {
    if (!EngineState::getInstance().isPlaying()) {
        return;
    }

    if (!hasDeferredComponentStartup) {
        componentStartupDeferred = false;
        return;
    }

    componentStartupDeferred = false;
    hasDeferredComponentStartup = false;

    for (const auto& component : components) {
        startComponent(*component);

        if (destroyed) {
            break;
        }
    }

    syncTransformParent();
    syncChildTransformParents();
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

int Entity::getDisplayOrder() const {
    return displayOrder;
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
        if (component->started) {
            component->onEnable(this->enabled);
        }
    }
}

void Entity::setDisplayOrder(int order) {
    displayOrder = order;
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

void Entity::setPersistent(bool value) {
    persistent = value;
}

bool Entity::isPersistent() const {
    return persistent;
}

void Entity::restoreGuid(const std::string& guid) {
    this->guid = guid;
}

const std::string& Entity::getGuid() const {
    return guid;
}
