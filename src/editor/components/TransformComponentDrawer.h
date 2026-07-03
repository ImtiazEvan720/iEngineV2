#pragma once

#include "editor/components/IComponentDrawer.h"

class TransformComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float position[2] = {0.0f, 0.0f};
        float rotation = 0.0f;
    };

    EditState editState;
};
