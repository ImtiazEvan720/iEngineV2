#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "misc/Level.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/ProjectManager.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <cctype>
#include <filesystem>
#include <string>

namespace {
std::string sanitizePrefabName(const std::string& value) {
    std::string sanitized;
    sanitized.reserve(value.size());

    for (char character : value) {
        const unsigned char unsignedCharacter = static_cast<unsigned char>(character);
        if (std::isalnum(unsignedCharacter) || character == '-' || character == '_') {
            sanitized.push_back(character);
        } else if (std::isspace(unsignedCharacter)) {
            sanitized.push_back('_');
        }
    }

    if (sanitized.empty()) {
        sanitized = "Prefab";
    }

    return sanitized;
}

std::filesystem::path getPrefabPath(const std::string& prefabName) {
    std::filesystem::path path =
        ProjectManager::getInstance().getAssetsPath() / "Prefabs" / sanitizePrefabName(prefabName);
    path.replace_extension(".iprefab");
    return path;
}
}

bool EntityInspectorPanel::drawPrefabActions(Level& level, std::string& statusMessage) {
    Entity* selectedEntity = findSelectedEntity(level);
    if (selectedEntity == nullptr) {
        ImGui::BeginDisabled();
        ImGui::InputText("Prefab Name", &prefabName);
        ImGui::Button("Save As Prefab");
        ImGui::EndDisabled();
        ImGui::TextUnformatted("Select an entity to save it as a prefab.");
        return false;
    }

    if (prefabName.empty()) {
        prefabName = sanitizePrefabName(selectedEntity->getName());
    }

    ImGui::InputText("Prefab Name", &prefabName);
    ImGui::SameLine();

    if (ImGui::Button("Save As Prefab")) {
        const std::filesystem::path path = getPrefabPath(prefabName);
        std::string errorMessage;

        if (PrefabSerializer::saveEntity(*selectedEntity, path.string(), errorMessage)) {
            if (AssetManager::getInstance().loadAssetFile(path.string())) {
                statusMessage = "Saved prefab: " + path.string();
            } else {
                statusMessage = "Saved prefab, but failed to register it with AssetManager: " + path.string();
            }

            return true;
        }

        statusMessage = errorMessage.empty() ? "Failed to save prefab." : errorMessage;
    }

    ImGui::TextWrapped("Output: %s", getPrefabPath(prefabName).string().c_str());
    return false;
}
