#pragma once

#include "system/IRenderBackend.h"

#include "imgui.h"

#include <string>
#include <vector>

class ViewportGrid;

class PrefabPanel {
public:
    struct PrefabPreview {
        ImTextureID textureId{};
        ImVec2 uv0{};
        ImVec2 uv1{1.0f, 1.0f};
        float sourceWidth = 0.0f;
        float sourceHeight = 0.0f;
        float width = 0.0f;
        float height = 0.0f;
        bool valid = false;
    };

    void draw(std::string& statusMessage);
    void refreshPrefabs();
    void drawLevelDropTarget(
        const ViewportGrid& viewportGrid,
        const RenderRect& viewport,
        float toolbarHeight,
        std::string& statusMessage
    );

private:
    struct PrefabInfo {
        std::string name;
        std::string path;
        PrefabPreview preview;
    };

    bool prefabsScanned = false;
    int selectedPrefabIndex = -1;
    std::vector<PrefabInfo> prefabs;
};
