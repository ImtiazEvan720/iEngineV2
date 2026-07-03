#pragma once

#include "editor/components/IComponentDrawer.h"

class PlayerCameraComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float zoom = 1.0f;
        float viewportSize[2] = {1280.0f, 720.0f};
        float offset[2] = {0.0f, 0.0f};
        bool clampToBounds = false;
        float boundsMin[2] = {0.0f, 0.0f};
        float boundsMax[2] = {1280.0f, 720.0f};
    };

    EditState editState;
};
