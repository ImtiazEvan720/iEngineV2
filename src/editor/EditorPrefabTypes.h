#pragma once

namespace EditorPrefabDrag {
constexpr const char* PayloadType = "IENGINE_PREFAB";
constexpr int MaxPathLength = 512;
}

struct PrefabDragPayload {
    char path[EditorPrefabDrag::MaxPathLength] = {};
};
