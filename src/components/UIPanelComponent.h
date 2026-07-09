#pragma once

#include "components/Component.h"
#include "system/IRenderBackend.h"

class UIPanelComponent : public Component {
public:
    void setColor(const RenderColor& value);
    const RenderColor& getColor() const;
    void setOpacity(float value);
    float getOpacity() const;

    std::unique_ptr<Component> clone() const override;

private:
    RenderColor color{255, 255, 255, 255};
    float opacity = 1.0f;
};
