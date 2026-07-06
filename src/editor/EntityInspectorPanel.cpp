#include "editor/EntityInspectorPanel.h"

#include "Entity.h"
#include "misc/Level.h"

#include "imgui.h"

#include <cctype>
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

}

bool EntityInspectorPanel::draw(std::string& statusMessage) {
    Level& level = Level::getCurrentLevel();
    auto& entities = level.getEntities();
    bool prefabSaved = false;

    const std::string levelFileName = level.getSourceFileName();
    const std::string levelLabel = levelFileName.empty()
        ? "Current Level"
        : "Current Level (" + levelFileName + ")";

    ImGui::Text("Entities: %zu", entities.size());

    if (selectedEntityId >= 0) {
        ImGui::SameLine();
        ImGui::Text("Selected: %d", selectedEntityId);
    }

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::Separator();
    prefabSaved = drawPrefabActions(level, statusMessage);
    ImGui::Separator();

    if (entities.empty()) {
        ImGui::TextUnformatted("Current level has no entities.");
        return prefabSaved;
    }

    entityTree.draw(
        level,
        levelLabel,
        selectedEntityId,
        [this](Entity& entity, bool forceSync) {
            selectEntity(entity, forceSync);
        },
        [this](Entity& entity, bool forceSync) {
            syncEditStateFromEntity(entity, forceSync);
        },
        [this](Entity& entity, std::string& message) {
            drawEntityDetails(entity, message);
        },
        [this](int entityId) {
            pendingDeleteEntityId = entityId;
        },
        statusMessage
    );

    processPendingDelete(level, statusMessage);
    return prefabSaved;
}

int EntityInspectorPanel::getSelectedEntityId() const {
    return selectedEntityId;
}

void EntityInspectorPanel::selectEntity(Entity& entity, bool forceSync) {
    const bool changedSelection = selectedEntityId != entity.getId();
    selectedEntityId = entity.getId();
    if (changedSelection || prefabName.empty()) {
        prefabName = sanitizePrefabName(entity.getName());
    }

    syncEditStateFromEntity(entity, forceSync);
}

void EntityInspectorPanel::clearSelection() {
    selectedEntityId = -1;
    prefabName.clear();
    identityDrawer.clear();
}

void EntityInspectorPanel::requestDeleteSelected(std::string& statusMessage) {
    if (selectedEntityId < 0) {
        statusMessage = "No entity selected.";
        return;
    }

    pendingDeleteEntityId = selectedEntityId;
}

void EntityInspectorPanel::drawEntityDetails(Entity& entity, std::string& statusMessage) {
    ImGui::Text("Id: %d", entity.getId());
    ImGui::Text("State: %s", entity.isDestroyed() ? "Destroyed" : "Active");
    ImGui::Text("Enabled: %s", entity.isEnabled() ? "Yes" : "No");

    const bool isLocked = lockedEntityIndex == entity.getId();
    const char* buttonText = isLocked ? "Unlock Entity" : "Lock Entity";

    if (entity.getParent() == nullptr) {
        if (ImGui::Button(buttonText)) {
            if (isLocked) {
                lockedEntityIndex = -1;
                statusMessage = "Unlocked entity " + std::to_string(entity.getId()) + ".";
            } else {
                lockedEntityIndex = entity.getId();
                statusMessage = "Locked entity " + std::to_string(entity.getId()) + ".";
            }
        }
    }

    if (selectedEntityId == entity.getId() || lockedEntityIndex == entity.getId()) {
        syncEditStateFromEntity(entity);
        identityDrawer.draw(entity, statusMessage);
    } else {
        ImGui::Text("Name: %s", entity.getName().c_str());
        ImGui::Text("Tag: %s", entity.getTag().c_str());
    }

    componentList.draw(
        entity,
        selectedEntityId == entity.getId() || lockedEntityIndex == entity.getId(),
        [this](Entity& target, std::string& message) {
            drawAddComponentCombo(target, message);
        },
        [this](Entity& target, const char* componentName, std::string& message) {
            return drawRemoveComponentButton(target, componentName, message);
        },
        statusMessage
    );
}

Entity* EntityInspectorPanel::findSelectedEntity(Level& level) const {
    if (selectedEntityId < 0) {
        return nullptr;
    }

    return level.findEntityById(selectedEntityId);
}

void EntityInspectorPanel::processPendingDelete(Level& level, std::string& statusMessage) {
    if (pendingDeleteEntityId < 0) {
        return;
    }

    const int deletedId = pendingDeleteEntityId;
    pendingDeleteEntityId = -1;

    if (!level.destroyEntityByIdHierarchy(deletedId)) {
        statusMessage = "Failed to delete entity " + std::to_string(deletedId) + ".";
        return;
    }

    level.cleanupDestroyedEntities();

    if (selectedEntityId >= 0 && level.findEntityById(selectedEntityId) == nullptr) {
        clearSelection();
    }

    statusMessage = "Deleted entity " + std::to_string(deletedId) + ".";
}

void EntityInspectorPanel::syncEditStateFromEntity(Entity& entity, bool force) {
    identityDrawer.syncFromEntity(entity, force);
}
