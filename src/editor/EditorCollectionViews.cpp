#include "editor/EditorCollectionViews.h"

#include <algorithm>

namespace {
ImGuiChildFlags makeChildFlags(bool border) {
    return border ? ImGuiChildFlags_Borders : ImGuiChildFlags_None;
}

ImGuiWindowFlags makeGridWindowFlags(const GridViewOptions& options) {
    ImGuiWindowFlags flags = ImGuiWindowFlags_None;
    if (options.horizontalScrollbar) {
        flags |= ImGuiWindowFlags_HorizontalScrollbar;
    }

    return flags;
}

int calculateGridColumns(const GridViewOptions& options) {
    if (options.columns > 0) {
        return options.columns;
    }

    const float availableWidth = ImGui::GetContentRegionAvail().x;
    if (availableWidth <= 0.0f || options.cellWidth <= 0.0f) {
        return 1;
    }

    return std::max(1, static_cast<int>(availableWidth / options.cellWidth));
}

int calculateGridRows(int itemCount, int columns) {
    if (itemCount <= 0 || columns <= 0) {
        return 0;
    }

    return (itemCount + columns - 1) / columns;
}

int calculateFirstVisibleIndex(float scroll, float startPosition, float stride) {
    if (stride <= 0.0f) {
        return 0;
    }

    const float visibleOffset = std::max(0.0f, scroll - startPosition);
    return std::max(0, static_cast<int>(visibleOffset / stride) - 1);
}

int calculateLastVisibleIndex(float scroll, float visibleSize, float startPosition, float stride, int count) {
    if (stride <= 0.0f) {
        return count;
    }

    const float visibleOffset = std::max(0.0f, (scroll + visibleSize) - startPosition);
    return std::min(count, static_cast<int>(visibleOffset / stride) + 2);
}

void drawGridItemBadge(const GridViewItem& item, const ImVec2& itemMin, const ImVec2& itemMax) {
    if (item.badge.empty()) {
        return;
    }

    const ImVec2 textSize = ImGui::CalcTextSize(item.badge.c_str());
    const float labelHeight = textSize.y + 4.0f;
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    drawList->AddRectFilled(
        ImVec2(itemMin.x, itemMax.y - labelHeight),
        itemMax,
        item.badgeColor
    );
    drawList->AddText(
        ImVec2(
            itemMin.x + std::max(2.0f, ((itemMax.x - itemMin.x) - textSize.x) * 0.5f),
            itemMax.y - labelHeight + 2.0f
        ),
        IM_COL32(255, 255, 255, 245),
        item.badge.c_str()
    );
}

void drawGridDragPreview(const GridViewItem& item) {
    if (item.image == ImTextureID{}) {
        return;
    }

    const ImVec2 previewSize = item.dragPreviewSize.x > 0.0f && item.dragPreviewSize.y > 0.0f
        ? item.dragPreviewSize
        : item.imageSize;

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const ImVec2 halfSize(previewSize.x * 0.5f, previewSize.y * 0.5f);
    ImGui::GetForegroundDrawList()->AddImage(
        item.image,
        ImVec2(mousePosition.x - halfSize.x, mousePosition.y - halfSize.y),
        ImVec2(mousePosition.x + halfSize.x, mousePosition.y + halfSize.y),
        item.uv0,
        item.uv1
    );
}

void drawGridCellContents(
    const GridViewItem& item,
    const ImVec2& cellMin,
    const ImVec2& cellMax,
    bool selected,
    bool hovered
) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    if (selected) {
        drawList->AddRectFilled(cellMin, cellMax, ImGui::GetColorU32(ImGuiCol_HeaderActive), 4.0f);
    } else if (hovered) {
        drawList->AddRectFilled(cellMin, cellMax, ImGui::GetColorU32(ImGuiCol_HeaderHovered), 4.0f);
    }

    drawList->AddRect(cellMin, cellMax, ImGui::GetColorU32(ImGuiCol_Border), 4.0f);

    const float padding = 4.0f;
    const float labelSpacing = 4.0f;
    const float labelHeight = item.label.empty() ? 0.0f : ImGui::GetTextLineHeight();
    const float availableImageWidth = std::max(
        1.0f,
        (cellMax.x - cellMin.x) - (padding * 2.0f)
    );
    const float availableImageHeight = std::max(
        1.0f,
        (cellMax.y - cellMin.y) - labelHeight - labelSpacing - (padding * 2.0f)
    );

    const ImVec2 imageSize(
        std::min(item.imageSize.x, availableImageWidth),
        std::min(item.imageSize.y, availableImageHeight)
    );

    const ImVec2 imageMin(
        cellMin.x + std::max(padding, ((cellMax.x - cellMin.x) - imageSize.x) * 0.5f),
        cellMin.y + padding
    );
    const ImVec2 imageMax(imageMin.x + imageSize.x, imageMin.y + imageSize.y);

    if (item.image != ImTextureID{}) {
        drawList->AddImage(
            item.image,
            imageMin,
            imageMax,
            item.uv0,
            item.uv1,
            item.disabled ? IM_COL32(255, 255, 255, 120) : IM_COL32_WHITE
        );
        drawList->AddRect(imageMin, imageMax, ImGui::GetColorU32(ImGuiCol_Border));
        drawGridItemBadge(item, imageMin, imageMax);
    }

    if (!item.label.empty()) {
        const ImVec2 labelMin(cellMin.x + padding, cellMax.y - labelHeight - padding);
        const ImVec2 labelMax(cellMax.x - padding, cellMax.y - padding);
        drawList->PushClipRect(labelMin, labelMax, true);
        drawList->AddText(
            labelMin,
            ImGui::GetColorU32(item.disabled ? ImGuiCol_TextDisabled : ImGuiCol_Text),
            item.label.c_str()
        );
        drawList->PopClipRect();
    }
}
}

ListViewResult EditorCollectionViews::drawListView(
    const char* id,
    const std::vector<ListViewItem>& items,
    int& selectedIndex,
    const ListViewOptions& options
) {
    ListViewResult result;

    if (ImGui::BeginChild(id, ImVec2(0.0f, options.height), makeChildFlags(options.border))) {
        if (items.empty()) {
            ImGui::TextDisabled("%s", options.emptyText);
        }

        for (int index = 0; index < static_cast<int>(items.size()); ++index) {
            const ListViewItem& item = items[static_cast<std::size_t>(index)];
            ImGui::PushID(item.id.empty() ? item.label.c_str() : item.id.c_str());

            if (item.disabled) {
                ImGui::BeginDisabled();
            }

            const bool selected = selectedIndex == index;
            if (ImGui::Selectable(item.label.c_str(), selected)) {
                selectedIndex = index;
                result.clickedIndex = index;
                result.selectionChanged = true;
            }

            if (!item.tooltip.empty() && ImGui::IsItemHovered()) {
                ImGui::SetTooltip("%s", item.tooltip.c_str());
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                result.doubleClickedIndex = index;
            }

            if (item.disabled) {
                ImGui::EndDisabled();
            }

            ImGui::PopID();
        }
    }

    ImGui::EndChild();
    return result;
}

GridViewResult EditorCollectionViews::drawGridView(
    const char* id,
    const std::vector<GridViewItem>& items,
    int& selectedIndex,
    const GridViewOptions& options
) {
    GridViewResult result;
    const int columns = calculateGridColumns(options);
    const float cellWidth = std::max(1.0f, options.cellWidth);
    const float cellHeight = std::max(1.0f, options.cellHeight);
    const float columnStride = cellWidth + ImGui::GetStyle().ItemSpacing.x;
    const float rowStride = cellHeight + ImGui::GetStyle().ItemSpacing.y;

    if (ImGui::BeginChild(
            id,
            ImVec2(0.0f, options.height),
            makeChildFlags(options.border),
            makeGridWindowFlags(options))) {
        if (items.empty()) {
            ImGui::TextDisabled("%s", options.emptyText);
        }

        const int itemCount = static_cast<int>(items.size());
        if (itemCount > 0) {
            const int rows = calculateGridRows(itemCount, columns);
            const ImVec2 startCursor = ImGui::GetCursorPos();
            const float totalWidth = (static_cast<float>(columns) * cellWidth)
                + (static_cast<float>(std::max(0, columns - 1)) * ImGui::GetStyle().ItemSpacing.x);
            const float totalHeight = (static_cast<float>(rows) * cellHeight)
                + (static_cast<float>(std::max(0, rows - 1)) * ImGui::GetStyle().ItemSpacing.y);

            const int firstVisibleRow = calculateFirstVisibleIndex(
                ImGui::GetScrollY(),
                startCursor.y,
                rowStride
            );
            const int lastVisibleRow = calculateLastVisibleIndex(
                ImGui::GetScrollY(),
                ImGui::GetWindowHeight(),
                startCursor.y,
                rowStride,
                rows
            );
            const int firstVisibleColumn = calculateFirstVisibleIndex(
                ImGui::GetScrollX(),
                startCursor.x,
                columnStride
            );
            const int lastVisibleColumn = calculateLastVisibleIndex(
                ImGui::GetScrollX(),
                ImGui::GetWindowWidth(),
                startCursor.x,
                columnStride,
                columns
            );

            for (int row = firstVisibleRow; row < lastVisibleRow; ++row) {
                for (int column = firstVisibleColumn; column < lastVisibleColumn; ++column) {
                    const int index = (row * columns) + column;
                    if (index < 0 || index >= itemCount) {
                        continue;
                    }

                    const GridViewItem& item = items[static_cast<std::size_t>(index)];
                    ImGui::PushID(item.id.empty() ? item.label.c_str() : item.id.c_str());
                    ImGui::SetCursorPos(ImVec2(
                        startCursor.x + (static_cast<float>(column) * columnStride),
                        startCursor.y + (static_cast<float>(row) * rowStride)
                    ));

                    if (item.disabled) {
                        ImGui::BeginDisabled();
                    }

                    const bool selected = selectedIndex == index;
                    const bool clicked = ImGui::InvisibleButton("##GridCell", ImVec2(cellWidth, cellHeight));
                    const bool hovered = ImGui::IsItemHovered();
                    const ImVec2 itemMin = ImGui::GetItemRectMin();
                    const ImVec2 itemMax = ImGui::GetItemRectMax();

                    if (clicked) {
                        selectedIndex = index;
                        result.clickedIndex = index;
                        result.selectionChanged = true;
                    }

                    if (!item.tooltip.empty() && hovered) {
                        ImGui::SetTooltip("%s", item.tooltip.c_str());
                    }

                    if (hovered && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                        result.doubleClickedIndex = index;
                    }

                    drawGridCellContents(item, itemMin, itemMax, selected, hovered);

                    if (item.dragPayloadType != nullptr
                        && !item.dragPayload.empty()
                        && ImGui::BeginDragDropSource(ImGuiDragDropFlags_SourceAllowNullID)) {
                        ImGui::SetDragDropPayload(
                            item.dragPayloadType,
                            item.dragPayload.data(),
                            item.dragPayload.size()
                        );

                        if (!item.dragLabel.empty()) {
                            ImGui::TextUnformatted(item.dragLabel.c_str());
                        } else {
                            ImGui::TextUnformatted(item.label.c_str());
                        }

                        drawGridDragPreview(item);
                        ImGui::EndDragDropSource();
                    }

                    if (item.disabled) {
                        ImGui::EndDisabled();
                    }

                    ImGui::PopID();
                }
            }

            ImGui::SetCursorPos(startCursor);
            ImGui::Dummy(ImVec2(totalWidth, totalHeight));
        }
    }

    ImGui::EndChild();
    return result;
}
