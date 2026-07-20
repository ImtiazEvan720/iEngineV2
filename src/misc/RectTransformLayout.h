#pragma once

#include "components/RectTransformComponent.h"
#include "math/Math2D.h"
#include "system/IRenderBackend.h"

#include <algorithm>
#include <cmath>
#include <string>

namespace RectTransformLayout {

inline Vector2F getAnchorNormalized(AnchorAlignment alignment) {
    switch (alignment) {
        case AnchorAlignment::LeftTop:
            return Vector2F(0.0f, 0.0f);
        case AnchorAlignment::LeftCenter:
            return Vector2F(0.0f, 0.5f);
        case AnchorAlignment::LeftBottom:
            return Vector2F(0.0f, 1.0f);
        case AnchorAlignment::CenterTop:
            return Vector2F(0.5f, 0.0f);
        case AnchorAlignment::Center:
            return Vector2F(0.5f, 0.5f);
        case AnchorAlignment::CenterBottom:
            return Vector2F(0.5f, 1.0f);
        case AnchorAlignment::RightTop:
            return Vector2F(1.0f, 0.0f);
        case AnchorAlignment::RightCenter:
            return Vector2F(1.0f, 0.5f);
        case AnchorAlignment::RightBottom:
            return Vector2F(1.0f, 1.0f);
    }

    return Vector2F::zero();
}

inline const char* anchorAlignmentToString(AnchorAlignment alignment) {
    switch (alignment) {
        case AnchorAlignment::LeftTop:
            return "LeftTop";
        case AnchorAlignment::LeftCenter:
            return "LeftCenter";
        case AnchorAlignment::LeftBottom:
            return "LeftBottom";
        case AnchorAlignment::CenterTop:
            return "CenterTop";
        case AnchorAlignment::Center:
            return "Center";
        case AnchorAlignment::CenterBottom:
            return "CenterBottom";
        case AnchorAlignment::RightTop:
            return "RightTop";
        case AnchorAlignment::RightCenter:
            return "RightCenter";
        case AnchorAlignment::RightBottom:
            return "RightBottom";
    }

    return "LeftTop";
}

inline bool anchorAlignmentFromString(const std::string& value, AnchorAlignment& output) {
    if (value == "LeftTop") {
        output = AnchorAlignment::LeftTop;
        return true;
    }
    if (value == "LeftCenter") {
        output = AnchorAlignment::LeftCenter;
        return true;
    }
    if (value == "LeftBottom") {
        output = AnchorAlignment::LeftBottom;
        return true;
    }
    if (value == "CenterTop") {
        output = AnchorAlignment::CenterTop;
        return true;
    }
    if (value == "Center") {
        output = AnchorAlignment::Center;
        return true;
    }
    if (value == "CenterBottom") {
        output = AnchorAlignment::CenterBottom;
        return true;
    }
    if (value == "RightTop" || value == "RighTop") {
        output = AnchorAlignment::RightTop;
        return true;
    }
    if (value == "RightCenter") {
        output = AnchorAlignment::RightCenter;
        return true;
    }
    if (value == "RightBottom") {
        output = AnchorAlignment::RightBottom;
        return true;
    }

    return false;
}

inline Vector2F getRootAnchorPoint(
    AnchorAlignment alignment,
    const RenderRect& viewport
) {
    const Vector2F normalized = getAnchorNormalized(alignment);
    return Vector2F(
        viewport.x + (viewport.width * normalized.x),
        viewport.y + (viewport.height * normalized.y)
    );
}

inline Vector2F getParentAnchorOffset(
    const RectTransformComponent& parent,
    AnchorAlignment childAlignment,
    float scale
) {
    const Vector2F normalized = getAnchorNormalized(childAlignment);
    const Vector2F parentSize = parent.getSize() * scale;
    const Vector2F& parentPivot = parent.getPivot();

    return Vector2F(
        parentSize.x * (normalized.x - parentPivot.x),
        parentSize.y * (normalized.y - parentPivot.y)
    );
}

inline float getSafeScale(float scale) {
    return std::isfinite(scale) ? std::max(0.0f, scale) : 1.0f;
}

inline Vector2F resolveWorldPosition(
    const RectTransformComponent& rectTransform,
    const RenderRect& viewport,
    float scale
) {
    const RectTransformComponent* parent = rectTransform.getParent();
    if (parent == nullptr) {
        return getRootAnchorPoint(rectTransform.getAnchorAlignment(), viewport)
            + rectTransform.getAnchoredPosition();
    }

    const float safeScale = getSafeScale(scale);
    const Vector2F parentPosition = resolveWorldPosition(*parent, viewport, safeScale);
    const float parentRotation = parent->getWorldRotation();
    const Vector2F localPosition =
        getParentAnchorOffset(*parent, rectTransform.getAnchorAlignment(), safeScale)
        + rectTransform.getAnchoredPosition();

    return parentPosition + Math2D::rotate(localPosition, parentRotation);
}

inline Vector2F worldToAnchoredPosition(
    const RectTransformComponent& rectTransform,
    const Vector2F& worldPosition,
    const RenderRect& viewport,
    float scale
) {
    const RectTransformComponent* parent = rectTransform.getParent();
    if (parent == nullptr) {
        return worldPosition - getRootAnchorPoint(rectTransform.getAnchorAlignment(), viewport);
    }

    const float safeScale = getSafeScale(scale);
    const Vector2F parentPosition = resolveWorldPosition(*parent, viewport, safeScale);
    const float parentRotation = parent->getWorldRotation();
    const Vector2F localPosition =
        Math2D::inverseRotate(worldPosition - parentPosition, parentRotation);

    return localPosition - getParentAnchorOffset(*parent, rectTransform.getAnchorAlignment(), safeScale);
}

inline RenderRect buildBounds(
    const RectTransformComponent& rectTransform,
    const RenderRect& viewport,
    float scale
) {
    const Vector2F screenPosition = resolveWorldPosition(rectTransform, viewport, scale);
    const Vector2F& size = rectTransform.getSize();
    const Vector2F& pivot = rectTransform.getPivot();
    const float safeScale = getSafeScale(scale);
    const float width = size.x * safeScale;
    const float height = size.y * safeScale;

    if (!std::isfinite(width) || !std::isfinite(height) || width <= 0.0f || height <= 0.0f) {
        return RenderRect{};
    }

    const float left = -(width * pivot.x);
    const float top = -(height * pivot.y);
    const float right = left + width;
    const float bottom = top + height;
    const float rotation = rectTransform.getWorldRotation();
    const Vector2F corners[] = {
        screenPosition + Math2D::rotate(Vector2F(left, top), rotation),
        screenPosition + Math2D::rotate(Vector2F(right, top), rotation),
        screenPosition + Math2D::rotate(Vector2F(right, bottom), rotation),
        screenPosition + Math2D::rotate(Vector2F(left, bottom), rotation)
    };

    float minX = corners[0].x;
    float maxX = corners[0].x;
    float minY = corners[0].y;
    float maxY = corners[0].y;
    for (const Vector2F& corner : corners) {
        minX = std::min(minX, corner.x);
        maxX = std::max(maxX, corner.x);
        minY = std::min(minY, corner.y);
        maxY = std::max(maxY, corner.y);
    }

    return RenderRect{
        minX,
        minY,
        maxX - minX,
        maxY - minY
    };
}

}
