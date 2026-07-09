#pragma once

#include "editor/components/IComponentDrawer.h"

#include <string>

class CollisionComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float offset[2] = {0.0f, 0.0f};
        float size[2] = {0.0f, 0.0f};
        float rotation = 0.0f;
        int bodyType = 0;
        bool sensor = false;
        bool fixedRotation = false;
        std::string name;
    };

    EditState editState;
};
