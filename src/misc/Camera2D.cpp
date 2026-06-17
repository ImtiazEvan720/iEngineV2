#include "misc/Camera2D.h"

#include <algorithm>
#include <cmath>

namespace {
bool isFinite(float value) {
    return std::isfinite(value);
}

bool isFinite(const Vector2F& value) {
    return isFinite(value.x) && isFinite(value.y);
}

bool isUsableViewport(const RenderRect& viewport) {
    return isFinite(viewport.x)
        && isFinite(viewport.y)
        && isFinite(viewport.width)
        && isFinite(viewport.height)
        && viewport.width > 0.0f
        && viewport.height > 0.0f;
}
}

const Vector2F& Camera2D::getPosition() const {
    return position;
}

float Camera2D::getZoom() const {
    return clampZoom(zoom);
}

void Camera2D::setPosition(const Vector2F& position) {
    this->position = isFinite(position) ? position : Vector2F::zero();
}

void Camera2D::setZoom(float zoom) {
    this->zoom = clampZoom(zoom);
}

void Camera2D::zoomAtScreenPoint(float zoomMultiplier, const Vector2F& screenPoint, const RenderRect& viewport) {
    if (!isFinite(zoomMultiplier)
        || zoomMultiplier <= 0.0f
        || !isFinite(screenPoint)
        || !isUsableViewport(viewport)) {
        return;
    }

    zoom = clampZoom(zoom);
    if (!isFinite(position)) {
        position = Vector2F::zero();
    }

    const float targetZoom = clampZoom(zoom * zoomMultiplier);
    if (targetZoom == zoom) {
        return;
    }

    const Vector2F worldBeforeZoom = screenToWorld(screenPoint, viewport);
    zoom = targetZoom;
    const Vector2F worldAfterZoom = screenToWorld(screenPoint, viewport);

    position.x += worldBeforeZoom.x - worldAfterZoom.x;
    position.y += worldBeforeZoom.y - worldAfterZoom.y;
}

Vector2F Camera2D::screenToWorld(const Vector2F& screenPoint, const RenderRect& viewport) const {
    const float effectiveZoom = clampZoom(zoom);
    const Vector2F effectivePosition = isFinite(position) ? position : Vector2F::zero();

    if (!isFinite(screenPoint) || !isUsableViewport(viewport)) {
        return effectivePosition;
    }

    return Vector2F(
        ((screenPoint.x - viewport.x) / effectiveZoom) + effectivePosition.x,
        ((screenPoint.y - viewport.y) / effectiveZoom) + effectivePosition.y
    );
}

Vector2F Camera2D::worldToScreen(const Vector2F& worldPoint, const RenderRect& viewport) const {
    const float effectiveZoom = clampZoom(zoom);
    const Vector2F effectivePosition = isFinite(position) ? position : Vector2F::zero();

    if (!isFinite(worldPoint) || !isUsableViewport(viewport)) {
        return Vector2F::zero();
    }

    return Vector2F(
        viewport.x + ((worldPoint.x - effectivePosition.x) * effectiveZoom),
        viewport.y + ((worldPoint.y - effectivePosition.y) * effectiveZoom)
    );
}

RenderRect Camera2D::getWorldViewport(const RenderRect& viewport) const {
    if (!isUsableViewport(viewport)) {
        return RenderRect{};
    }

    const float effectiveZoom = clampZoom(zoom);
    const Vector2F topLeft = screenToWorld(Vector2F(viewport.x, viewport.y), viewport);

    return RenderRect{
        topLeft.x,
        topLeft.y,
        viewport.width / effectiveZoom,
        viewport.height / effectiveZoom
    };
}

float Camera2D::clampZoom(float zoom) {
    if (!std::isfinite(zoom)) {
        return 1.0f;
    }

    return std::clamp(zoom, minZoom, maxZoom);
}
