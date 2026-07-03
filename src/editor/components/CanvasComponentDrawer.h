#pragma once

#include "editor/components/IComponentDrawer.h"
#include "system/IRenderBackend.h"

class CanvasComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float referenceResolution[2] = {1280.0f, 720.0f};
        int sortingOrder = 0;
        float opacity = 1.0f;
        float scale = 1.0f;
        RenderColor canvasColor = {0, 0, 0, 255};
    };

    EditState editState;
    bool screenEyedropActive = false;
    int screenEyedropTargetEntityId = -1;
    int screenEyedropArmedFrame = -1;
};
