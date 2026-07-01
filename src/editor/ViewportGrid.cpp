#include "editor/ViewportGrid.h"

#include "misc/Camera2D.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <algorithm>
#include <cmath>
#include <string>

void ViewportGrid::drawMenuItems() {
    ImGui::MenuItem("Show Grid", nullptr, &showGrid);
    ImGui::MenuItem("Show Colliders", nullptr, &showColliders);
    ImGui::MenuItem("Snap to Grid", nullptr, &snapToGrid);
    ImGui::MenuItem("Show Level Editor", nullptr, &showSideMenu);
    ImGui::MenuItem("Show Preview Camera", nullptr, &showPreviewCamera);
}

void ViewportGrid::draw(const RenderRect& viewport) const {
    constexpr int MaxGridLinesPerAxis = 512;
    constexpr int MaxGridLabels = 1200;
    constexpr float MinScreenTileSizeForLabels = 32.0f;

    Renderer& renderer = Renderer::getInstance();
    const Camera2D& camera = renderer.getCamera();
    const RenderRect worldViewport = camera.getWorldViewport(viewport);
    const float tileSize = getGridSize();
    const float screenTileSize = tileSize * camera.getZoom();

    if (tileSize <= 0.0f
        || screenTileSize <= 0.0f
        || viewport.width <= 0.0f
        || viewport.height <= 0.0f
        || !std::isfinite(worldViewport.x)
        || !std::isfinite(worldViewport.y)
        || !std::isfinite(worldViewport.width)
        || !std::isfinite(worldViewport.height)) {
        return;
    }

    ImDrawList* gridDrawList = ImGui::GetBackgroundDrawList();
    ImDrawList* textDrawList = ImGui::GetForegroundDrawList();

    const ImU32 lineColor = IM_COL32(255, 255, 255, 35);
    const ImU32 textColor = IM_COL32(255, 255, 255, 90);

    const float worldLeft = worldViewport.x;
    const float worldTop = worldViewport.y;
    const float worldRight = worldViewport.x + worldViewport.width;
    const float worldBottom = worldViewport.y + worldViewport.height;

    const int startColumn = static_cast<int>(std::floor(worldLeft / tileSize));
    const int endColumn = static_cast<int>(std::ceil(worldRight / tileSize));
    const int startRow = static_cast<int>(std::floor(worldTop / tileSize));
    const int endRow = static_cast<int>(std::ceil(worldBottom / tileSize));
    const int visibleColumns = std::max(1, endColumn - startColumn);
    const int visibleRows = std::max(1, endRow - startRow);

    if (visibleColumns > MaxGridLinesPerAxis || visibleRows > MaxGridLinesPerAxis) {
        return;
    }

    for (int row = startRow; row <= endRow; ++row) {
        const float worldY = static_cast<float>(row) * tileSize;
        const Vector2F start = camera.worldToScreen(Vector2F(worldLeft, worldY), viewport);
        const Vector2F end = camera.worldToScreen(Vector2F(worldRight, worldY), viewport);
        gridDrawList->AddLine(
            ImVec2(start.x, start.y),
            ImVec2(end.x, end.y),
            lineColor
        );
    }

    for (int column = startColumn; column <= endColumn; ++column) {
        const float worldX = static_cast<float>(column) * tileSize;
        const Vector2F start = camera.worldToScreen(Vector2F(worldX, worldTop), viewport);
        const Vector2F end = camera.worldToScreen(Vector2F(worldX, worldBottom), viewport);
        gridDrawList->AddLine(
            ImVec2(start.x, start.y),
            ImVec2(end.x, end.y),
            lineColor
        );
    }

    const int totalVisibleCells = visibleColumns * visibleRows;
    if (screenTileSize < MinScreenTileSizeForLabels || totalVisibleCells > MaxGridLabels) {
        return;
    }

    for (int row = startRow; row < endRow; ++row) {
        for (int column = startColumn; column < endColumn; ++column) {
            const int id = (row - startRow) * visibleColumns + (column - startColumn);
            const float worldX = static_cast<float>(column) * tileSize;
            const float worldY = static_cast<float>(row) * tileSize;
            const Vector2F screenPosition = camera.worldToScreen(Vector2F(worldX, worldY + tileSize), viewport);

            if (screenPosition.x < viewport.x - screenTileSize || screenPosition.x > viewport.x + viewport.width) {
                continue;
            }

            if (screenPosition.y < viewport.y || screenPosition.y > viewport.y + viewport.height + screenTileSize) {
                continue;
            }

            const ImVec2 textPosition(
                screenPosition.x + 4.0f,
                screenPosition.y - ImGui::GetTextLineHeight() - 3.0f
            );

            textDrawList->AddText(
                textPosition,
                textColor,
                std::to_string(id).c_str()
            );
        }
    }
}

bool ViewportGrid::shouldShowGrid() const {
    return showGrid;
}

bool ViewportGrid::shouldShowColliders() const {
    return showColliders;
}

bool ViewportGrid::shouldSnapToGrid() const {
    return snapToGrid;
}

bool ViewportGrid::shouldShowSideMenu() const {
    return showSideMenu;
}

Vector2F ViewportGrid::snapPosition(const Vector2F& position) const {
    const float gridSize = getGridSize();
    if (gridSize <= 0.0f) {
        return position;
    }

    const float halfGridSize = gridSize * 0.5f;
    return Vector2F(
        std::round((position.x - halfGridSize) / gridSize) * gridSize + halfGridSize,
        std::round((position.y - halfGridSize) / gridSize) * gridSize + halfGridSize
    );
}

float ViewportGrid::getGridSize() const {
    return 16.0f * Renderer::getInstance().getRenderScale();
}

bool ViewportGrid::shouldShowPreviewCamera() const {
    return showPreviewCamera;
}
