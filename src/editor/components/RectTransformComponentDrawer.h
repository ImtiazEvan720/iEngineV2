#pragma once

#include "editor/components/IComponentDrawer.h"

class RectTransformComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float anchoredPosition[2] = {0.0f, 0.0f};
        float size[2] = {100.0f, 100.0f};
        float pivot[2] = {0.5f, 0.5f};
        float rotation = 0.0f;
    };

    EditState editState;
};
