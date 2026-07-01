#pragma once

#include "components/Component.h"
#include "math/Vector2F.h"

class PlayerCameraComponent : public Component {
public:
    PlayerCameraComponent() = default;
    PlayerCameraComponent(
        float zoom,
        float viewportWidth,
        float viewportHeight,
        const Vector2F& offset,
        bool clampToBounds,
        float minX,
        float minY,
        float maxX,
        float maxY
    );

    float getZoom() const;
    float getViewportWidth() const;
    float getViewportHeight() const;
    const Vector2F& getOffset() const;
    bool shouldClampToBounds() const;
    float getMinX() const;
    float getMinY() const;
    float getMaxX() const;
    float getMaxY() const;

    void setZoom(float zoom);
    void setViewportWidth(float viewportWidth);
    void setViewportHeight(float viewportHeight);
    void setOffset(const Vector2F& offset);
    void setClampToBounds(bool clampToBounds);
    void setBounds(float minX, float minY, float maxX, float maxY);
    std::unique_ptr<Component> clone() const override;

private:
    float zoom = 1.0f;
    float viewportWidth = 1280.0f;
    float viewportHeight = 720.0f;
    Vector2F offset = Vector2F::zero();
    bool clampToBounds = false;
    float minX = 0.0f;
    float minY = 0.0f;
    float maxX = 1280.0f;
    float maxY = 720.0f;
};
