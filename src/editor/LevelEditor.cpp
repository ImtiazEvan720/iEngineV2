#include "editor/LevelEditor.h"

#include "components/TransformComponent.h"
#include "components/RectTransformComponent.h"
#include "components/CanvasComponent.h"
#include "editor/BuildSystem.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "system/EngineState.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"
#include "system/RawInputSystem.h"
#include "system/Renderer.h"
#include "system/TouchControlSystem.h"
#include "system/VirtualInputSystem.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <filesystem>
#include <utility>

namespace {
std::filesystem::path resolveLevelSavePath(const std::string& fileName) {
    namespace fs = std::filesystem;

    fs::path outputFileName(fileName);
    outputFileName = outputFileName.filename();
    if (outputFileName.empty()) {
        return {};
    }

    if (outputFileName.extension() != ".ilevel") {
        outputFileName.replace_extension(".ilevel");
    }

    return ProjectManager::getInstance().getAssetsPath() / "Levels" / outputFileName;
}

std::string getBuildScriptCommand(const std::string& scriptName) {
    namespace fs = std::filesystem;

    if (fs::exists(scriptName)) {
        return "./" + scriptName;
    }

    if (fs::exists(fs::path("..") / ".." / scriptName)) {
        return "cd ../.. && ./" + scriptName;
    }

    return "./" + scriptName;
}

Entity* findEntityById(Level& level, int entityId) {
    for (Entity& entity : level.getEntities()) {
        if (entity.getId() == entityId && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}
}

void LevelEditor::draw(const InputSystem& inputSystem, float windowWidth) {
    (void)windowWidth;

    if (ImGui::BeginMainMenuBar()) {
        drawProjectMenu();

        if (ImGui::BeginMenu("Level")) {
            if (ImGui::MenuItem("Create")) {
                Level::createNewLevel();
                Renderer::getInstance().clearTileLayerBatches();
                entityInspector.clearSelection();
                statusMessage = "Created new empty level.";
            }

            if (ImGui::MenuItem("Save...")) {
                openLevelSaveWindow();
            }

            if (ImGui::MenuItem("Load...")) {
                showLevelLoadWindow = true;
            }

            if (ImGui::MenuItem("Delete")) {
                // Delete/close the active level once level management is added.
            }

            if (ImGui::MenuItem("Reload")) {
                // Reload the active .ilevel once editor loading is added.
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("Entity")) {
            if (ImGui::MenuItem("Create Empty")) {
                const RenderRect viewport = camera.getViewport();
                const RenderRect worldViewport = Renderer::getInstance().getCamera().getWorldViewport(viewport);
                Vector2F position(
                    worldViewport.x + (worldViewport.width * 0.5f),
                    worldViewport.y + (worldViewport.height * 0.5f)
                );

                if (viewportGrid.shouldSnapToGrid()) {
                    position = viewportGrid.snapPosition(position);
                }

                Entity& entity = Level::getCurrentLevel().createEntity();
                entity.setName("Empty Entity");
                entity.setTag("Default");
                entity.addComponent<TransformComponent>(position, 0.0f);
                entityInspector.selectEntity(entity, true);
                statusMessage = "Created empty entity " + std::to_string(entity.getId()) + ".";
            }

            if (ImGui::BeginMenu("UI")) {
                if (ImGui::MenuItem("Canvas")) {
                    const RenderRect viewport = Renderer::getInstance().getViewport();
                    Vector2F position(
                        viewport.x + (viewport.width * 0.5f),
                        viewport.y + (viewport.height * 0.5f)
                    );

                    Entity& entity = Level::getCurrentLevel().createEntity();
                    entity.setName("Canvas");
                    entity.setTag("UI");
                    entity.addComponent<RectTransformComponent>(position, Vector2F(640.0f, 320.0f), Vector2F(0.5f, 0.5f), 0.0f);
                    entity.addComponent<CanvasComponent>();
                    entityInspector.selectEntity(entity, true);
                    statusMessage = "Created Canvas entity " + std::to_string(entity.getId()) + ".";
                }

                ImGui::EndMenu();
            }

            const bool hasSelectedEntity = entityInspector.getSelectedEntityId() >= 0;
            if (ImGui::MenuItem("Duplicate Selected", nullptr, false, hasSelectedEntity)) {
                duplicateSelectedEntity();
            }

            if (ImGui::MenuItem("Delete Selected", nullptr, false, hasSelectedEntity)) {
                entityInspector.requestDeleteSelected(statusMessage);
            }

            ImGui::EndMenu();
        }

        drawAssetsMenu();
        drawBuildMenu();

        if (ImGui::BeginMenu("Tools")) {
            ImGui::MenuItem("Editor Enabled", nullptr, &enabled);
            ImGui::Separator();

            if (ImGui::MenuItem("Select", nullptr, currentTool == Tool::Select, enabled)) {
                currentTool = Tool::Select;
            }

            if (ImGui::MenuItem("Move", nullptr, currentTool == Tool::Move, enabled)) {
                currentTool = Tool::Move;
            }

            ImGui::EndMenu();
        }

        if (ImGui::BeginMenu("View")) {
            viewportGrid.drawMenuItems();
            ImGui::EndMenu();
        }


        drawGameControls();

        ImGui::EndMainMenuBar();
    }

    entityInspector.processPendingDelete(Level::getCurrentLevel(), statusMessage);

    if (tilesetCreator.draw(statusMessage)) {
        spritePalette.refreshTilesets();
    }

    drawLevelSaveWindow();
    drawLevelLoadWindow();
    drawProjectNewWindow();
    drawProjectOpenWindow();
    drawBuildOutputWindow();

    if (viewportGrid.shouldShowSideMenu()) {
        const bool levelEditorOpen = ImGui::Begin("Level Editor");

        if (levelEditorOpen && ImGui::BeginTabBar("##LevelEditorTabs")) {
            if (ImGui::BeginTabItem("Level_Outline")) {
                drawLevelOutlineTab(inputSystem);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Entities")) {
                if (entityInspector.draw(statusMessage)) {
                    prefabPanel.refreshPrefabs();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Sprites")) {
                spritePalette.draw(statusMessage);
                ImGui::EndTabItem();
            }
            if (ImGui::BeginTabItem("Animations")) {
                animationEditorPanel.draw(statusMessage);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Prefabs")) {
                prefabPanel.draw(statusMessage);
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Levels")) {
                if (levelManagerPanel.draw(statusMessage)) {
                    Renderer::getInstance().clearTileLayerBatches();
                    entityInspector.clearSelection();
                }
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("File_Explorer")) {
                drawFileExplorerTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::End();
    }

    if (viewport.draw(
            enabled,
            camera,
            viewportGrid,
            entityInspector,
            spritePalette,
            prefabPanel,
            getToolbarHeight(),
            statusMessage)) {
        currentTool = Tool::Move;
    }

    if (viewportGrid.shouldShowPreviewCamera()) {
        playCameraPreviewPanel.draw(statusMessage);
    }
}

void LevelEditor::updateEditorOnly(float deltaTime) {
    (void)deltaTime;

    camera.beginFrame();
    const bool zoomActive = camera.handleZoom(enabled, statusMessage);
    const bool panActive = camera.handlePan(enabled);
    const bool dragDropActive = ImGui::GetDragDropPayload() != nullptr;
    const bool gridVisible =
        enabled && (viewportGrid.shouldShowGrid() || viewportGrid.shouldShowColliders());
    const bool cameraActive = enabled && (zoomActive || panActive || camera.isPanActive());

    Renderer::getInstance().setEditorViewportActivity(
        dragDropActive,
        gridVisible,
        cameraActive
    );
}

void LevelEditor::duplicateSelectedEntity() {
    Level& level = Level::getCurrentLevel();
    Entity* selectedEntity = findEntityById(level, entityInspector.getSelectedEntityId());
    if (selectedEntity == nullptr) {
        statusMessage = "No entity selected.";
        return;
    }

    Entity* duplicate = level.duplicateEntity(*selectedEntity, true);
    if (duplicate == nullptr) {
        statusMessage = "Failed to duplicate entity.";
        return;
    }

    entityInspector.selectEntity(*duplicate, true);
    statusMessage = "Duplicated entity " + std::to_string(selectedEntity->getId())
        + " as " + std::to_string(duplicate->getId()) + ".";
}

void LevelEditor::setEnabled(bool value) {
    enabled = value;
}

bool LevelEditor::isEnabled() const {
    return enabled;
}

void LevelEditor::toggleEnabled() {
    enabled = !enabled;
}

LevelEditor::Tool LevelEditor::getCurrentTool() const {
    return currentTool;
}

float LevelEditor::getToolbarHeight() const {
    if (ImGui::GetCurrentContext() == nullptr) {
        return 0.0f;
    }

    return ImGui::GetFrameHeight();
}

bool LevelEditor::shouldShowGrid() const {
    return viewportGrid.shouldShowGrid();
}

bool LevelEditor::shouldShowColliders() const {
    return viewportGrid.shouldShowColliders();
}

bool LevelEditor::shouldSnapToGrid() const {
    return viewportGrid.shouldSnapToGrid();
}

void LevelEditor::drawLevelOutlineTab(const InputSystem& inputSystem) {
    const RawInputSystem& rawInput = RawInputSystem::getInstance();
    const TouchControlSystem& touchControls = TouchControlSystem::getInstance();
    const VirtualInputSystem& virtualInput = VirtualInputSystem::getInstance();

    ImGui::Text("Input: %s", inputSystem.getLastInputText().c_str());
    ImGui::Text("Editor: %s", enabled ? "Enabled" : "Disabled");
    ImGui::Text("Tool: %s", currentTool == Tool::Select ? "Select" : "Move");
    ImGui::Separator();
    ImGui::Text("Raw Mouse: %d, %d", rawInput.getMouseX(), rawInput.getMouseY());
    ImGui::Text(
        "Raw Keys: W=%s A=%s S=%s D=%s Space=%s Escape=%s",
        rawInput.isKeyDown(RawKey::W) ? "down" : "up",
        rawInput.isKeyDown(RawKey::A) ? "down" : "up",
        rawInput.isKeyDown(RawKey::S) ? "down" : "up",
        rawInput.isKeyDown(RawKey::D) ? "down" : "up",
        rawInput.isKeyDown(RawKey::Space) ? "down" : "up",
        rawInput.isKeyDown(RawKey::Escape) ? "down" : "up"
    );
    ImGui::Text(
        "Raw Mouse Buttons: Left=%s Right=%s Middle=%s",
        rawInput.isMouseButtonDown(RawMouseButton::Left) ? "down" : "up",
        rawInput.isMouseButtonDown(RawMouseButton::Right) ? "down" : "up",
        rawInput.isMouseButtonDown(RawMouseButton::Middle) ? "down" : "up"
    );
    ImGui::Text("Raw Touches: %zu", rawInput.getTouches().size());
    ImGui::Text(
        "Touch Controls: %s Up=%s Down=%s Left=%s Right=%s Fire=%s",
        touchControls.isEnabled() ? "enabled" : "disabled",
        touchControls.isControlDown(TouchControl::MoveStickUp) ? "down" : "up",
        touchControls.isControlDown(TouchControl::MoveStickDown) ? "down" : "up",
        touchControls.isControlDown(TouchControl::MoveStickLeft) ? "down" : "up",
        touchControls.isControlDown(TouchControl::MoveStickRight) ? "down" : "up",
        touchControls.isControlDown(TouchControl::FireButton) ? "down" : "up"
    );
    ImGui::Text(
        "Actions: Up=%s Down=%s Left=%s Right=%s Fire=%s Fire2=%s",
        virtualInput.isActionDown(InputAction::MoveUp) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveDown) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveLeft) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveRight) ? "down" : "up",
        virtualInput.isActionDown(InputAction::Fire) ? "down" : "up",
        virtualInput.isActionDown(InputAction::Fire2) ? "down" : "up"
    );
}

void LevelEditor::drawLevelLoadWindow() {
    if (!showLevelLoadWindow) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 320.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Load Level", &showLevelLoadWindow)) {
        ImGui::End();
        return;
    }

    const std::filesystem::path levelsPath =
        ProjectManager::getInstance().getAssetsPath() / "Levels";
    ImGui::TextWrapped("%s", levelsPath.string().c_str());
    ImGui::Separator();

    const std::vector<std::filesystem::path> levelFiles = getLevelFiles();
    if (levelFiles.empty()) {
        ImGui::TextDisabled("No .ilevel files found.");
    }

    for (const std::filesystem::path& path : levelFiles) {
        const bool selected = path.string() == Level::getCurrentLevelPath();
        if (ImGui::Selectable(path.filename().string().c_str(), selected)) {
            loadSelectedLevel(path.string());
            showLevelLoadWindow = false;
        }
    }

    if (!statusMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::End();
}

void LevelEditor::drawLevelSaveWindow() {
    if (!showLevelSaveWindow) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(420.0f, 180.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Save Level", &showLevelSaveWindow)) {
        ImGui::End();
        return;
    }

    const std::filesystem::path levelsPath =
        ProjectManager::getInstance().getAssetsPath() / "Levels";
    ImGui::TextWrapped("%s", levelsPath.string().c_str());
    ImGui::Separator();
    ImGui::InputText("File Name", &saveLevelFileName);

    const std::filesystem::path outputPath = resolveLevelSavePath(saveLevelFileName);
    if (!outputPath.empty()) {
        ImGui::TextWrapped("Save path: %s", outputPath.string().c_str());
    }

    if (ImGui::Button("Save")) {
        saveLevelToPromptPath();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        showLevelSaveWindow = false;
    }

    if (!statusMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::End();
}

void LevelEditor::openLevelSaveWindow() {
    const std::filesystem::path currentPath(Level::getCurrentLevelPath());
    if (!currentPath.filename().empty()) {
        saveLevelFileName = currentPath.filename().string();
    }

    showLevelSaveWindow = true;
}

void LevelEditor::saveLevelToPromptPath() {
    const std::filesystem::path outputPath = resolveLevelSavePath(saveLevelFileName);
    if (outputPath.empty() || outputPath.stem().empty()) {
        statusMessage = "Level file name cannot be empty.";
        return;
    }

    std::string errorMessage;
    if (!Level::saveCurrentLevel(outputPath.string(), errorMessage)) {
        statusMessage = errorMessage.empty() ? "Failed to save level." : errorMessage;
        return;
    }

    statusMessage = "Saved level: " + Level::getCurrentLevelPath();
    showLevelSaveWindow = false;
}

void LevelEditor::loadSelectedLevel(const std::string& path) {
    Level loadedLevel = Level::createEmpty();
    std::string errorMessage;

    if (!Level::loadFromFile(path, loadedLevel, errorMessage)) {
        statusMessage = errorMessage.empty() ? "Failed to load level." : errorMessage;
        return;
    }

    Level::loadLevel(std::move(loadedLevel));
    Renderer::getInstance().clearTileLayerBatches();
    entityInspector.clearSelection();
    statusMessage = "Loaded level: " + Level::getCurrentLevelPath();
}

void LevelEditor::drawAssetsMenu() {
    if (!ImGui::BeginMenu("Assets")) {
        return;
    }

    if (ImGui::MenuItem("Create Tileset")) {
        tilesetCreator.open();
    }

    if (ImGui::MenuItem("Refresh Tilesets")) {
        spritePalette.refreshTilesets();
        statusMessage = "Refreshed tilesets.";
    }

    if (ImGui::MenuItem("Refresh Prefabs")) {
        prefabPanel.refreshPrefabs();
        statusMessage = "Refreshed prefabs.";
    }

    ImGui::EndMenu();
}

void LevelEditor::drawBuildMenu() {
    if (!ImGui::BeginMenu("Build")) {
        return;
    }

    BuildSystem& buildSystem = BuildSystem::getInstance();
    const bool canStartBuild = !buildSystem.isRunning();

    if (ImGui::MenuItem("Desktop Release", nullptr, false, canStartBuild)) {
        startBuild("desktop release", "build_release.sh");
    }

    if (ImGui::MenuItem("Web", nullptr, false, canStartBuild)) {
        startBuild("web", "build_web.sh");
    }

    if (ImGui::MenuItem("iOS Simulator", nullptr, false, canStartBuild)) {
        startBuild("iOS simulator", "build_ios.sh");
    }

    if (ImGui::MenuItem("Android Debug APK", nullptr, false, canStartBuild)) {
        startBuild("Android debug APK", "build_android.sh");
    }

    ImGui::Separator();

    if (ImGui::MenuItem("Show Output")) {
        showBuildOutputWindow = true;
    }

    if (ImGui::MenuItem("Clear Output", nullptr, false, canStartBuild)) {
        buildSystem.clearOutput();
    }

    ImGui::EndMenu();
}

void LevelEditor::drawGameControls() {
    EngineState& engineState = EngineState::getInstance();
    const EngineState::RuntimeMode runtimeMode = engineState.getRuntimeMode();
    const bool isEdit = runtimeMode == EngineState::RuntimeMode::Edit;
    const bool isPlaying = runtimeMode == EngineState::RuntimeMode::Play;
    const bool isPaused = runtimeMode == EngineState::RuntimeMode::Pause;

    ImGui::Separator();

    if (isPaused) {
        if (ImGui::Button("Resume")) {
            startPlayMode();
        }
    } else {
        if (!isEdit) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Play")) {
            startPlayMode();
        }

        if (!isEdit) {
            ImGui::EndDisabled();
        }
    }

    ImGui::SameLine();

    if (!isPlaying) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Pause")) {
        pausePlayMode();
    }

    if (!isPlaying) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();

    if (!(isPlaying || isPaused)) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Stop")) {
        stopPlayMode();
    }

    if (!(isPlaying || isPaused)) {
        ImGui::EndDisabled();
    }

    ImGui::Separator();
}

void LevelEditor::startBuild(const std::string& label, const std::string& scriptName) {
    BuildSystem& buildSystem = BuildSystem::getInstance();

    showBuildOutputWindow = true;
    if (buildSystem.run(getBuildScriptCommand(scriptName))) {
        statusMessage = "Started " + label + " build.";
    } else {
        statusMessage = "A build is already running.";
    }
}

void LevelEditor::startPlayMode() {
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Play);
    Level::startPendingComponents();
    statusMessage = "Play mode started.";
}

void LevelEditor::pausePlayMode() {
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Pause);
    statusMessage = "Play mode paused.";
}

void LevelEditor::stopPlayMode() {
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Edit);

    Level loadedLevel = Level::createEmpty();
    std::string errorMessage;
    const std::string currentLevelPath = Level::getCurrentLevelPath();

    if (!currentLevelPath.empty()
        && Level::loadFromFile(currentLevelPath, loadedLevel, errorMessage)) {
        Level::loadLevel(std::move(loadedLevel));
        Renderer::getInstance().clearTileLayerBatches();
        entityInspector.clearSelection();
        statusMessage = "Stopped play mode and reloaded: " + Level::getCurrentLevelPath();
    } else {
        statusMessage =
            "Stopped play mode. Failed to reload level: "
            + (errorMessage.empty() ? currentLevelPath : errorMessage);
    }
}

void LevelEditor::drawBuildOutputWindow() {
    if (!showBuildOutputWindow) {
        return;
    }

    BuildSystem& buildSystem = BuildSystem::getInstance();

    ImGui::SetNextWindowSize(ImVec2(720.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Build Output", &showBuildOutputWindow)) {
        ImGui::End();
        return;
    }

    ImGui::Text("Status: %s", buildSystem.getStatusText().c_str());

    const bool canStartBuild = !buildSystem.isRunning();
    if (ImGui::Button("Build Desktop Release")) {
        startBuild("desktop release", "build_release.sh");
    }

    ImGui::SameLine();
    if (ImGui::Button("Build Web")) {
        startBuild("web", "build_web.sh");
    }

    ImGui::SameLine();
    if (ImGui::Button("Build iOS")) {
        startBuild("iOS simulator", "build_ios.sh");
    }

    ImGui::SameLine();
    if (ImGui::Button("Build Android")) {
        startBuild("Android debug APK", "build_android.sh");
    }

    ImGui::SameLine();
    if (ImGui::Button("Clear") && canStartBuild) {
        buildSystem.clearOutput();
    }

    ImGui::Separator();

    const std::string output = buildSystem.getOutput();
    if (ImGui::BeginChild(
            "##BuildOutputLog",
            ImVec2(0.0f, 0.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        ImGui::TextUnformatted(output.c_str());

        if (buildSystem.isRunning()) {
            ImGui::SetScrollHereY(1.0f);
        }
    }

    ImGui::EndChild();
    ImGui::End();
}
