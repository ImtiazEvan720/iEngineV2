#pragma once

#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

class Camera2D;

class RectGizmo {
public:
    RectGizmo() = default;
    ~RectGizmo() = default;

    enum class Handle {
        None,
        Move,
        ResizeLeft,
        ResizeRight,
        ResizeTop,
        ResizeBottom,
        ResizeTopLeft,
        ResizeTopRight,
        ResizeBottomLeft,
        ResizeBottomRight
    };

    struct Rect {
        Vector2F center = Vector2F::zero();
        float width = 0.0f;
        float height = 0.0f;
        float rotation = 0.0f;
    };

    struct EditResult {
        bool active = false;
        bool changed = false;
        bool finished = false;
        Rect rect;
        Handle handle = Handle::None;
    };

    struct Options {
        bool allowMove = true;
        bool allowResize = true;
    };

    void cancel();

    void drawHandles(
        const Rect& rect,
        const Camera2D& camera,
        const RenderRect& viewport
    ) const;
    void drawHandlesScreenSpace(
        const Rect& rect,
        const RenderRect& viewport
    ) const;

    Handle hitTest(
        const Rect& rect,
        const Camera2D& camera,
        const RenderRect& viewport,
        const Vector2F& screenMousePosition
    ) const;
    Handle hitTestScreenSpace(
        const Rect& rect,
        const RenderRect& viewport,
        const Vector2F& screenMousePosition
    ) const;

    EditResult handleInteraction(
        int id,
        const Rect& rect,
        const Camera2D& camera,
        const RenderRect& viewport,
        const Vector2F& worldMousePosition,
        bool snapToGrid,
        const Vector2F& gridSize,
        const Options& options
    );
    EditResult handleInteractionScreenSpace(
        int id,
        const Rect& rect,
        const RenderRect& viewport,
        const Vector2F& screenMousePosition,
        bool snapToGrid,
        const Vector2F& gridSize,
        const Options& options
    );

private:
    int editingId = -1;
    Handle activeHandle = Handle::None;
    Vector2F dragOffset = Vector2F::zero();
};
