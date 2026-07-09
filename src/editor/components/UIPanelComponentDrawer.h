#pragma once

#include "editor/components/IComponentDrawer.h"

class UIPanelComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;
};
