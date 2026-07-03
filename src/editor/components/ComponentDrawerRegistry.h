#pragma once

#include "editor/components/IComponentDrawer.h"

#include <memory>
#include <string>
#include <typeindex>
#include <unordered_map>

class Component;

class ComponentDrawerRegistry {
public:
    ComponentDrawerRegistry();

    IComponentDrawer* getDrawer(Component& component);
    const char* getComponentName(Component& component) const;

private:
    struct Entry {
        std::string name;
        std::unique_ptr<IComponentDrawer> drawer;
    };

    template <typename TComponent>
    void registerDrawer(const std::string& name, std::unique_ptr<IComponentDrawer> drawer) {
        drawers.emplace(
            std::type_index(typeid(TComponent)),
            Entry{name, std::move(drawer)}
        );
    }

    std::unordered_map<std::type_index, Entry> drawers;
};
