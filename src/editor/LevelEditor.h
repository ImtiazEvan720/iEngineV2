#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/PrefabPanel.h"
#include "editor/SpritePalettePanel.h"
#include "editor/TilesetCreator.h"
#include "editor/ViewportGrid.h"

#include <string>

class InputSystem;
class Entity;

class LevelEditor {
public:
    enum class Tool {
        Select,
        Move
    };

    void draw(const InputSystem& inputSystem, float windowWidth);

    void setEnabled(bool value);
    bool isEnabled() const;
    void toggleEnabled();

    Tool getCurrentTool() const;
    float getToolbarHeight() const;
    bool shouldShowGrid() const;
    bool shouldShowColliders() const;
    bool shouldSnapToGrid() const;

private:
    void drawLevelOutlineTab(const InputSystem& inputSystem);
    void drawFileExplorerTab();
    void drawAssetsMenu();
    void handleViewportEntityInteraction();
    Entity* findEntityAt(float x, float y);
    Entity* findEntityById(int id);
    bool entityContainsPoint(Entity& entity, float x, float y) const;
    
    bool enabled = true;
    int draggingEntityId = -1;
    Tool currentTool = Tool::Select;
    float dragOffset[2] = {0.0f, 0.0f};
    std::string statusMessage;
    EditorCamera camera;
    EntityInspectorPanel entityInspector;
    SpritePalettePanel spritePalette;
    PrefabPanel prefabPanel;
    TilesetCreator tilesetCreator;
    ViewportGrid viewportGrid;
};

#endif
