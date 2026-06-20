#ifndef IENGINEV2_LEVELEDITOR_H
#define IENGINEV2_LEVELEDITOR_H

#include "editor/EditorCamera.h"
#include "editor/EntityInspectorPanel.h"
#include "editor/LevelEditorViewport.h"
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
    void updateEditorOnly(float deltaTime);

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
    void drawProjectMenu();
    void drawProjectOpenWindow();
    void drawProjectNewWindow();
    void drawAssetsMenu();
    void drawBuildMenu();
    void drawGameControls();
    void drawBuildOutputWindow();
    void startBuild(const std::string& label, const std::string& scriptName);
    void startPlayMode();
    void pausePlayMode();
    void stopPlayMode();
    void openProjectNewWindow();
    void openProjectOpenWindow();
    void createNewProject();
    void openSelectedProject();
    std::vector<std::filesystem::path> getLevelFiles() const;
    void openLevelSaveWindow();
    void saveLevelToPromptPath();
    void loadSelectedLevel(const std::string& path);

    bool enabled = true;
    bool showLevelLoadWindow = false;
    bool showLevelSaveWindow = false;
    bool showProjectNewWindow = false;
    bool showProjectOpenWindow = false;
    bool showBuildOutputWindow = false;
    Tool currentTool = Tool::Select;
    std::string statusMessage;
    std::string saveLevelFileName = "current.ilevel";
    std::string newProjectParentPath;
    std::string newProjectName = "NewProject";
    std::string projectBrowserPath;
    std::string selectedProjectPath;
    EditorCamera camera;
    EntityInspectorPanel entityInspector;
    SpritePalettePanel spritePalette;
    PrefabPanel prefabPanel;
    TilesetCreator tilesetCreator;
    LevelEditorViewport viewport;
    ViewportGrid viewportGrid;
};

#endif
