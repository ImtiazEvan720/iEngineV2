#pragma once

#include "editor/components/ComponentDrawerRegistry.h"

#include <functional>
#include <string>

class Entity;

class EntityComponentList {
public:
    using AddComponentCallback = std::function<void(Entity&, std::string&)>;
    using RemoveComponentCallback = std::function<bool(Entity&, const char*, std::string&)>;

    bool draw(
        Entity& entity,
        bool editable,
        AddComponentCallback addComponentCallback,
        RemoveComponentCallback removeComponentCallback,
        std::string& statusMessage
    );

private:
    ComponentDrawerRegistry componentDrawerRegistry;
};
