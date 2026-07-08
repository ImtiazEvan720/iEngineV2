#pragma once
#include "components/Component.h"
#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

namespace {
constexpr RenderColor DEFAULT_CANVAS_COLOR = {180, 180, 180, 220};
}
class CanvasComponent: public Component {
public:
    CanvasComponent() = default;
    ~CanvasComponent() = default;
    std::unique_ptr<Component> clone() const override;

    void setReferenceResolution(const Vector2F& resolution);
    void setSortingOrder(int order);
    void setOpacity(float opacity);
    void setScale(float scale);
    void setCanvasColor(const RenderColor& color);

    const Vector2F& getReferenceResolution() const;
    int getSortingOrder() const;
    float getOpacity() const;
    float getScale() const;
    const RenderColor& getCanvasColor() const;


private:
    Vector2F ReferenceResolution = Vector2F(640.0f, 320.0f);
    int SortingOrder = 0;
    float Opacity = 1.0f;
    float Scale = 1.0f;
    RenderColor CanvasColor = DEFAULT_CANVAS_COLOR;
};
