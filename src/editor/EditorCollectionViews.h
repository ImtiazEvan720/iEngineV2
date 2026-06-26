#pragma once

#include "imgui.h"

#include <string>
#include <vector>

struct ListViewItem {
    std::string id;
    std::string label;
    std::string tooltip;
    bool disabled = false;
};

struct ListViewResult {
    int clickedIndex = -1;
    int doubleClickedIndex = -1;
    bool selectionChanged = false;
};

struct ListViewOptions {
    float height = 0.0f;
    bool border = true;
    bool closePopupOnSelection = false;
    const char* emptyText = "No items.";
};

struct GridViewItem {
    std::string id;
    std::string label;
    std::string tooltip;

    ImTextureID image = ImTextureID{};
    ImVec2 imageSize = ImVec2(48.0f, 48.0f);
    ImVec2 uv0 = ImVec2(0.0f, 0.0f);
    ImVec2 uv1 = ImVec2(1.0f, 1.0f);

    std::string badge;
    ImU32 badgeColor = IM_COL32(64, 64, 64, 220);

    const char* dragPayloadType = nullptr;
    std::vector<unsigned char> dragPayload;
    std::string dragLabel;
    ImVec2 dragPreviewSize = ImVec2(0.0f, 0.0f);

    bool disabled = false;
};

struct GridViewResult {
    int clickedIndex = -1;
    int doubleClickedIndex = -1;
    bool selectionChanged = false;
};

struct GridViewOptions {
    float height = 0.0f;
    float cellWidth = 72.0f;
    float cellHeight = 92.0f;
    int columns = 0;
    bool border = true;
    bool horizontalScrollbar = false;
    const char* emptyText = "No items.";
};

class EditorCollectionViews {
public:
    static ListViewResult drawListView(
        const char* id,
        const std::vector<ListViewItem>& items,
        int& selectedIndex,
        const ListViewOptions& options = {}
    );

    static GridViewResult drawGridView(
        const char* id,
        const std::vector<GridViewItem>& items,
        int& selectedIndex,
        const GridViewOptions& options = {}
    );
};
