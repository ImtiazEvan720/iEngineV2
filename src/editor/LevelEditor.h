#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/PrefabPanel.h"
#include "editor/SpritePalettePanel.h"
#include "editor/TilesetCreator.h"
#include "editor/ViewportGrid.h"

#include <filesystem>
#include <string>
#include <vector>

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
    void drawLevelLoadWindow();
    void drawLevelSaveWindow();
    void drawAssetsMenu();
    std::vector<std::filesystem::path> getLevelFiles() const;
    void openLevelSaveWindow();
    void saveLevelToPromptPath();
    void loadSelectedLevel(const std::string& path);
    void handleViewportEntityInteraction();
    Entity* findEntityAt(float x, float y);
    Entity* findEntityById(int id);
    bool entityContainsPoint(Entity& entity, float x, float y) const;
    
    bool enabled = true;
    bool showLevelLoadWindow = false;
    bool showLevelSaveWindow = false;
    int draggingEntityId = -1;
    Tool currentTool = Tool::Select;
    float dragOffset[2] = {0.0f, 0.0f};
    std::string statusMessage;
    std::string saveLevelFileName = "current.ilevel";
    EditorCamera camera;
    EntityInspectorPanel entityInspector;
    SpritePalettePanel spritePalette;
    PrefabPanel prefabPanel;
    TilesetCreator tilesetCreator;
    ViewportGrid viewportGrid;
};

#endif
