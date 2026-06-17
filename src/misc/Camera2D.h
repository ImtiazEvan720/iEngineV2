#ifndef IENGINEV2_CAMERA2D_H
#define IENGINEV2_CAMERA2D_H

#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

class Camera2D {
public:
    static constexpr float minZoom = 0.25f;
    static constexpr float maxZoom = 4.0f;

    const Vector2F& getPosition() const;
    float getZoom() const;

    void setPosition(const Vector2F& position);
    void setZoom(float zoom);
    void zoomAtScreenPoint(float zoomMultiplier, const Vector2F& screenPoint, const RenderRect& viewport);

    Vector2F screenToWorld(const Vector2F& screenPoint, const RenderRect& viewport) const;
    Vector2F worldToScreen(const Vector2F& worldPoint, const RenderRect& viewport) const;
    RenderRect getWorldViewport(const RenderRect& viewport) const;

private:
    static float clampZoom(float zoom);

    Vector2F position = Vector2F::zero();
    float zoom = 1.0f;
};

#endif
