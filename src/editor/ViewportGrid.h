#pragma once

#include "math/Vector2F.h"
#include "system/IRenderBackend.h"

class ViewportGrid {
public:
    void drawMenuItems();
    void draw(const RenderRect& viewport) const;

    bool shouldShowGrid() const;
    bool shouldShowColliders() const;
    bool shouldShowSideMenu() const;
    bool shouldSnapToGrid() const;

    Vector2F snapPosition(const Vector2F& position) const;
    float getGridSize() const;

private:
    bool showGrid = false;
    bool snapToGrid = false;
    bool showColliders = false;
    bool showSideMenu = true;
};
