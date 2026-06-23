#pragma once

#include "components/Component.h"

#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

class InputSystem;
class Vector2F;

class Entity {
public:
    Entity();
    ~Entity();

    Entity(const Entity& other) = delete;
    Entity& operator=(const Entity& other) = delete;
    Entity(Entity&& other) noexcept;
    Entity& operator=(Entity&& other) noexcept;

    template <typename TComponent, typename... Args>
    TComponent& addComponent(Args&&... args) {
        static_assert(std::is_base_of<Component, TComponent>::value,
                      "TComponent must inherit from Component");

        auto component = std::make_unique<TComponent>(std::forward<Args>(args)...);
        TComponent& reference = *component;

        reference.setEntity(this);
        components.push_back(std::move(component));
        if (shouldStartComponentsImmediately()) {
            startComponent(reference);
        } else {
            hasDeferredComponentStartup = true;
        }
        syncTransformParent();
        syncChildTransformParents();

        return reference;
    }

    template <typename TComponent>
    TComponent* getComponent() {
        static_assert(std::is_base_of<Component, TComponent>::value,
                      "TComponent must inherit from Component");

        for (const auto& component : components) {
            if (auto* match = dynamic_cast<TComponent*>(component.get())) {
                return match;
            }
        }

        return nullptr;
    }

    template <typename TComponent>
    const TComponent* getComponent() const {
        static_assert(std::is_base_of<Component, TComponent>::value,
                      "TComponent must inherit from Component");

        for (const auto& component : components) {
            if (const auto* match = dynamic_cast<const TComponent*>(component.get())) {
                return match;
            }
        }

        return nullptr;
    }

    template <typename TComponent>
    bool removeComponent() {
        static_assert(std::is_base_of<Component, TComponent>::value,
                      "TComponent must inherit from Component");

        for (auto it = components.begin(); it != components.end(); ++it) {
            if (dynamic_cast<TComponent*>(it->get()) != nullptr) {
                if ((*it)->started) {
                    (*it)->onDestroy();
                    (*it)->started = false;
                }
                components.erase(it);
                syncTransformParent();
                syncChildTransformParents();
                return true;
            }
        }

        return false;
    }

    void update(float deltaTime);
    void destroy(bool destroyChildren = true);
    bool isDestroyed() const;
    void addInputListeners(InputSystem& inputSystem);
    void removeInputListeners(InputSystem& inputSystem);
    void setComponentStartupDeferred(bool deferred);
    void startDeferredComponents();

    int getId() const;
    const std::string& getName() const;
    const std::string& getTag() const;
    bool isEnabled() const;
    int getDisplayOrder() const;
    Entity* getParent();
    const Entity* getParent() const;
    const std::vector<Entity*>& getChildren() const;
    bool isChildOf(const Entity& possibleParent) const;

    void setName(const std::string& name);
    void setTag(const std::string& tag);
    void setEnabled(bool enabled);
    void setDisplayOrder(int order);
    bool setParent(Entity* parent, bool keepWorldTransform = true);
    void clearParent(bool keepWorldTransform = true);
    void setPersistent(bool value);
    bool isPersistent() const;
    const std::string& getGuid() const;

    static Entity* spawnPrefab(const std::string& prefabName, const Vector2F& position, float rotation = 0.0f);
    static Entity* spawnPrefab(const std::string& prefabName, float x, float y, float rotation = 0.0f);

private:
    friend class Level;

    static int globalId;

    void restoreGuid(const std::string& guid);
    void refreshComponentOwners();
    bool shouldStartComponentsImmediately() const;
    void startComponent(Component& component);
    void destroyComponents();
    void removeChildReference(Entity* child);
    void syncTransformParent();
    void syncChildTransformParents();
    void rebindParentLinksFrom(Entity* oldAddress);

    std::vector<std::unique_ptr<Component>> components;
    Entity* parentEntity = nullptr;
    std::vector<Entity*> children;
    std::string name;
    std::string tag;
    std::string guid;
    int id;
    int displayOrder;
    bool enabled = true;
    bool destroyed = false;
    bool updating = false;
    bool persistent = false;
    bool componentStartupDeferred = false;
    bool hasDeferredComponentStartup = false;
};
