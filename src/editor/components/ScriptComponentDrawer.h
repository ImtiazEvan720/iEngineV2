#pragma once

#include "editor/ScriptPropertyInspector.h"
#include "editor/components/IComponentDrawer.h"

class ScriptComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    ScriptPropertyInspector scriptPropertyInspector;
};
