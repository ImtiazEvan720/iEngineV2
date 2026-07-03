#pragma once

#include "editor/SpritePickerWidget.h"
#include "editor/components/IComponentDrawer.h"

class SpriteComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float source[4] = {0.0f, 0.0f, 0.0f, 0.0f};
        float size[2] = {0.0f, 0.0f};
        float origin[2] = {0.0f, 0.0f};
    };

    EditState editState;
    SpritePickerWidget spritePicker;
};
