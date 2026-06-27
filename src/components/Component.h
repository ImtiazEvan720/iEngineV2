#pragma once

#include <memory>

class Entity;

class Component {
public:
    virtual ~Component() = default;

    Entity* getEntity();
    const Entity* getEntity() const;

    virtual void onStart();
    virtual void onUpdate(float deltaTime);
    virtual void onEnable(bool value);
    virtual void onDestroy();
    virtual std::unique_ptr<Component> clone() const;

    bool isEnabled() const;
    void setEnabled(bool value);

private:
    friend class Entity;

    void setEntity(Entity* entity);

    Entity* entity = nullptr;
    bool started = false;
    bool enabled = true;
};
