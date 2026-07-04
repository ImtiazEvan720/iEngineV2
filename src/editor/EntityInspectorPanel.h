#pragma once

#include "editor/EntityComponentList.h"
#include "editor/EntityIdentityDrawer.h"
#include "editor/EntityTreePanel.h"

#include <string>

class Entity;
class Level;

class EntityInspectorPanel {
public:
    bool draw(std::string& statusMessage);

    int getSelectedEntityId() const;
    void selectEntity(Entity& entity, bool forceSync = false);
    void clearSelection();
    void requestDeleteSelected(std::string& statusMessage);
    void processPendingDelete(Level& level, std::string& statusMessage);
    void syncEditStateFromEntity(Entity& entity, bool force = false);

private:
    void drawEntityDetails(Entity& entity, std::string& statusMessage);
    void drawAddComponentCombo(Entity& entity, std::string& statusMessage);
    bool drawRemoveComponentButton(Entity& entity, const char* componentName, std::string& statusMessage);
    bool drawPrefabActions(Level& level, std::string& statusMessage);
    Entity* findSelectedEntity(Level& level) const;

    int selectedEntityId = -1;
    int pendingDeleteEntityId = -1;
    int selectedAddComponentIndex = 0;
    int selectedScriptIndex = 0;
    int lockedEntityIndex = -1;
    std::string prefabName;
    EntityIdentityDrawer identityDrawer;
    EntityComponentList componentList;
    EntityTreePanel entityTree;
};
