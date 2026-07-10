#pragma once

#include "editor/components/IComponentDrawer.h"

class UIButtonComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;
};
