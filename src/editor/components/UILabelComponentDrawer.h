#pragma once

#include "editor/components/IComponentDrawer.h"
#include "system/IRenderBackend.h"

#include <string>

class UILabelComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        std::string text;
        std::string fontName;
        int fontSize = 18;
        RenderColor fontColor{255, 255, 255, 255};
        int horizontalAlign = 1;
        int verticalAlign = 1;
    };

    EditState editState;
};
