#include "editor/LevelEditor.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/BuildSystem.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/EngineState.h"
#include "system/InputSystem.h"
#include "system/ProjectManager.h"
#include "system/RawInputSystem.h"
#include "system/Renderer.h"
#include "system/TouchControlSystem.h"
#include "system/VirtualInputSystem.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <filesystem>
#include <utility>

namespace {
constexpr float ColliderHandleSize = 8.0f;
constexpr float ColliderMinSize = 1.0f;

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

bool pointInRect(const Vector2F& point, const ImVec2& min, const ImVec2& max) {
    return point.x >= min.x
        && point.x <= max.x
        && point.y >= min.y
        && point.y <= max.y;
}

bool pointInHandle(const Vector2F& point, const ImVec2& center) {
    const float halfSize = ColliderHandleSize * 0.5f;
    return pointInRect(
        point,
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize)
    );
}

void addColliderHandle(ImDrawList& drawList, const ImVec2& center, ImU32 fillColor, ImU32 outlineColor) {
    const float halfSize = ColliderHandleSize * 0.5f;
    drawList.AddRectFilled(
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize),
        fillColor
    );
    drawList.AddRect(
        ImVec2(center.x - halfSize, center.y - halfSize),
        ImVec2(center.x + halfSize, center.y + halfSize),
        outlineColor
    );
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
                // Create an entity in the current level once editor commands are added.
            }

            if (ImGui::MenuItem("Delete Selected")) {
                // Delete the selected entity once selection is added.
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

            if (ImGui::BeginTabItem("Prefabs")) {
                prefabPanel.draw(statusMessage);
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

    Renderer& renderer = Renderer::getInstance();
    const bool activeDragDrop = ImGui::GetDragDropPayload() != nullptr;
    const bool activeEditorViewport =
        renderer.shouldRenderEditorViewport()
        || activeDragDrop
        || (enabled && (viewportGrid.shouldShowGrid() || viewportGrid.shouldShowColliders()));

    if (activeEditorViewport && viewportGrid.shouldShowGrid()) {
        viewportGrid.draw(camera.getViewport());
    }

    if (activeEditorViewport && viewportGrid.shouldShowColliders()) {
        drawSelectedColliderBounds();
    }

    handleViewportEntityInteraction();
    if (activeEditorViewport) {
        spritePalette.drawLevelDropTarget(
            viewportGrid,
            camera.getViewport(),
            getToolbarHeight(),
            statusMessage
        );
        prefabPanel.drawLevelDropTarget(
            viewportGrid,
            camera.getViewport(),
            getToolbarHeight(),
            statusMessage
        );
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
        "Raw Keys: W=%s A=%s S=%s D=%s Space=%s",
        rawInput.isKeyDown(RawKey::W) ? "down" : "up",
        rawInput.isKeyDown(RawKey::A) ? "down" : "up",
        rawInput.isKeyDown(RawKey::S) ? "down" : "up",
        rawInput.isKeyDown(RawKey::D) ? "down" : "up",
        rawInput.isKeyDown(RawKey::Space) ? "down" : "up"
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
        "Actions: Up=%s Down=%s Left=%s Right=%s Fire=%s",
        virtualInput.isActionDown(InputAction::MoveUp) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveDown) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveLeft) ? "down" : "up",
        virtualInput.isActionDown(InputAction::MoveRight) ? "down" : "up",
        virtualInput.isActionDown(InputAction::Fire) ? "down" : "up"
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
    statusMessage = "Play mode started.";
}

void LevelEditor::pausePlayMode() {
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Pause);
    statusMessage = "Play mode paused.";
}

void LevelEditor::stopPlayMode() {
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

    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Edit);
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

void LevelEditor::drawSelectedColliderBounds() {
    const int selectedEntityId = entityInspector.getSelectedEntityId();
    if (selectedEntityId < 0) {
        return;
    }

    Entity* entity = findEntityById(selectedEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    const RenderRect viewport = camera.getViewport();
    if (viewport.width <= 0.0f || viewport.height <= 0.0f) {
        return;
    }

    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    Vector2F worldCenter = transform->getWorldPosition();
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;
    const float halfWidth = collider->getWidth() * 0.5f;
    const float halfHeight = collider->getHeight() * 0.5f;

    const Vector2F screenTopLeft = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x - halfWidth, worldCenter.y - halfHeight),
        viewport
    );
    const Vector2F screenBottomRight = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x + halfWidth, worldCenter.y + halfHeight),
        viewport
    );

    const float left = std::min(screenTopLeft.x, screenBottomRight.x);
    const float right = std::max(screenTopLeft.x, screenBottomRight.x);
    const float top = std::min(screenTopLeft.y, screenBottomRight.y);
    const float bottom = std::max(screenTopLeft.y, screenBottomRight.y);
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;

    ImDrawList* drawList = ImGui::GetForegroundDrawList();
    if (drawList == nullptr) {
        return;
    }

    const ImVec2 clipMin(viewport.x, viewport.y);
    const ImVec2 clipMax(viewport.x + viewport.width, viewport.y + viewport.height);
    const ImU32 outlineColor = IM_COL32(70, 210, 255, 235);
    const ImU32 fillColor = IM_COL32(70, 210, 255, 180);
    const ImU32 centerFillColor = IM_COL32(255, 220, 70, 210);
    const ImU32 shadowColor = IM_COL32(0, 0, 0, 130);

    drawList->PushClipRect(clipMin, clipMax, true);
    drawList->AddRect(ImVec2(left + 1.0f, top + 1.0f), ImVec2(right + 1.0f, bottom + 1.0f), shadowColor);
    drawList->AddRect(ImVec2(left, top), ImVec2(right, bottom), outlineColor, 0.0f, 0, 2.0f);

    addColliderHandle(*drawList, ImVec2(left, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, top), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(left, centerY), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, centerY), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(left, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(right, bottom), fillColor, outlineColor);
    addColliderHandle(*drawList, ImVec2(centerX, centerY), centerFillColor, outlineColor);
    drawList->PopClipRect();
}

bool LevelEditor::handleViewportColliderInteraction(
    const Vector2F& worldMousePosition,
    const RenderRect& viewport
) {
    if (!viewportGrid.shouldShowColliders()) {
        editingColliderEntityId = -1;
        colliderEditMode = ColliderEditMode::None;
        return false;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 mousePosition = ImGui::GetMousePos();
    const Vector2F screenMousePosition(mousePosition.x, mousePosition.y);

    if (colliderEditMode != ColliderEditMode::None) {
        if (editingColliderEntityId < 0 || findEntityById(editingColliderEntityId) == nullptr) {
            editingColliderEntityId = -1;
            colliderEditMode = ColliderEditMode::None;
            return false;
        }

        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            if (colliderEditMode == ColliderEditMode::Move) {
                moveSelectedCollider(worldMousePosition);
            } else {
                resizeSelectedCollider(worldMousePosition);
            }

            return true;
        }

        if (ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
            Entity* entity = findEntityById(editingColliderEntityId);
            if (entity != nullptr) {
                entityInspector.syncEditStateFromEntity(*entity, true);
                if (colliderEditMode == ColliderEditMode::Move) {
                    statusMessage = "Moved collider offset for entity "
                        + std::to_string(entity->getId()) + ".";
                } else {
                    statusMessage = "Updated collider bounds for entity "
                        + std::to_string(entity->getId()) + ".";
                }
            }

            editingColliderEntityId = -1;
            colliderEditMode = ColliderEditMode::None;
            return true;
        }

        return true;
    }

    if (!ImGui::IsMouseClicked(ImGuiMouseButton_Left) || io.WantCaptureMouse) {
        return false;
    }

    const ColliderEditMode hitMode = hitTestSelectedColliderHandle(screenMousePosition, viewport);
    if (hitMode == ColliderEditMode::None) {
        return false;
    }

    editingColliderEntityId = entityInspector.getSelectedEntityId();
    colliderEditMode = hitMode;
    draggingEntityId = -1;
    currentTool = Tool::Move;

    if (Entity* entity = findEntityById(editingColliderEntityId)) {
        if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
            if (CollisionComponent* collider = entity->getComponent<CollisionComponent>()) {
                Vector2F colliderWorldCenter = transform->getWorldPosition();
                const Vector2F& offset = collider->getOffset();
                colliderWorldCenter.x += offset.x;
                colliderWorldCenter.y += offset.y;
                dragOffset[0] = worldMousePosition.x - colliderWorldCenter.x;
                dragOffset[1] = worldMousePosition.y - colliderWorldCenter.y;
            }
        }
    }

    statusMessage = hitMode == ColliderEditMode::Move
        ? "Moving collider."
        : "Editing collider bounds.";
    return true;
}

LevelEditor::ColliderEditMode LevelEditor::hitTestSelectedColliderHandle(
    const Vector2F& screenMousePosition,
    const RenderRect& viewport
) {
    const int selectedEntityId = entityInspector.getSelectedEntityId();
    if (selectedEntityId < 0) {
        return ColliderEditMode::None;
    }

    Entity* entity = findEntityById(selectedEntityId);
    if (entity == nullptr) {
        return ColliderEditMode::None;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return ColliderEditMode::None;
    }

    const Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    Vector2F worldCenter = transform->getWorldPosition();
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;
    const float halfWidth = collider->getWidth() * 0.5f;
    const float halfHeight = collider->getHeight() * 0.5f;

    const Vector2F screenTopLeft = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x - halfWidth, worldCenter.y - halfHeight),
        viewport
    );
    const Vector2F screenBottomRight = sceneCamera.worldToScreen(
        Vector2F(worldCenter.x + halfWidth, worldCenter.y + halfHeight),
        viewport
    );

    const float left = std::min(screenTopLeft.x, screenBottomRight.x);
    const float right = std::max(screenTopLeft.x, screenBottomRight.x);
    const float top = std::min(screenTopLeft.y, screenBottomRight.y);
    const float bottom = std::max(screenTopLeft.y, screenBottomRight.y);
    const float centerX = (left + right) * 0.5f;
    const float centerY = (top + bottom) * 0.5f;

    if (pointInHandle(screenMousePosition, ImVec2(centerX, centerY))) {
        return ColliderEditMode::Move;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, top))) {
        return ColliderEditMode::ResizeTopLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, top))) {
        return ColliderEditMode::ResizeTopRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, bottom))) {
        return ColliderEditMode::ResizeBottomLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, bottom))) {
        return ColliderEditMode::ResizeBottomRight;
    }

    if (pointInHandle(screenMousePosition, ImVec2(centerX, top))) {
        return ColliderEditMode::ResizeTop;
    }

    if (pointInHandle(screenMousePosition, ImVec2(centerX, bottom))) {
        return ColliderEditMode::ResizeBottom;
    }

    if (pointInHandle(screenMousePosition, ImVec2(left, centerY))) {
        return ColliderEditMode::ResizeLeft;
    }

    if (pointInHandle(screenMousePosition, ImVec2(right, centerY))) {
        return ColliderEditMode::ResizeRight;
    }

    return ColliderEditMode::None;
}

void LevelEditor::moveSelectedCollider(const Vector2F& worldMousePosition) {
    Entity* entity = findEntityById(editingColliderEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    Vector2F colliderWorldCenter(
        worldMousePosition.x - dragOffset[0],
        worldMousePosition.y - dragOffset[1]
    );

    if (viewportGrid.shouldSnapToGrid()) {
        colliderWorldCenter = viewportGrid.snapPosition(colliderWorldCenter);
    }

    const Vector2F transformWorldPosition = transform->getWorldPosition();
    collider->setOffset(Vector2F(
        colliderWorldCenter.x - transformWorldPosition.x,
        colliderWorldCenter.y - transformWorldPosition.y
    ));
    entityInspector.syncEditStateFromEntity(*entity, true);
}

void LevelEditor::resizeSelectedCollider(const Vector2F& worldMousePosition) {
    Entity* entity = findEntityById(editingColliderEntityId);
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    CollisionComponent* collider = entity->getComponent<CollisionComponent>();
    if (transform == nullptr || collider == nullptr) {
        return;
    }

    const Vector2F transformWorldPosition = transform->getWorldPosition();
    Vector2F worldCenter = transformWorldPosition;
    const Vector2F& offset = collider->getOffset();
    worldCenter.x += offset.x;
    worldCenter.y += offset.y;

    float left = worldCenter.x - (collider->getWidth() * 0.5f);
    float right = worldCenter.x + (collider->getWidth() * 0.5f);
    float top = worldCenter.y - (collider->getHeight() * 0.5f);
    float bottom = worldCenter.y + (collider->getHeight() * 0.5f);
    Vector2F snappedMousePosition = worldMousePosition;

    if (viewportGrid.shouldSnapToGrid()) {
        snappedMousePosition = viewportGrid.snapPosition(worldMousePosition);
    }

    switch (colliderEditMode) {
        case ColliderEditMode::ResizeLeft:
        case ColliderEditMode::ResizeTopLeft:
        case ColliderEditMode::ResizeBottomLeft:
            left = std::min(snappedMousePosition.x, right - ColliderMinSize);
            break;
        case ColliderEditMode::ResizeRight:
        case ColliderEditMode::ResizeTopRight:
        case ColliderEditMode::ResizeBottomRight:
            right = std::max(snappedMousePosition.x, left + ColliderMinSize);
            break;
        case ColliderEditMode::ResizeTop:
        case ColliderEditMode::ResizeBottom:
        case ColliderEditMode::Move:
        case ColliderEditMode::None:
            break;
    }

    switch (colliderEditMode) {
        case ColliderEditMode::ResizeTop:
        case ColliderEditMode::ResizeTopLeft:
        case ColliderEditMode::ResizeTopRight:
            top = std::min(snappedMousePosition.y, bottom - ColliderMinSize);
            break;
        case ColliderEditMode::ResizeBottom:
        case ColliderEditMode::ResizeBottomLeft:
        case ColliderEditMode::ResizeBottomRight:
            bottom = std::max(snappedMousePosition.y, top + ColliderMinSize);
            break;
        case ColliderEditMode::ResizeLeft:
        case ColliderEditMode::ResizeRight:
        case ColliderEditMode::Move:
        case ColliderEditMode::None:
            break;
    }

    const float width = std::max(ColliderMinSize, right - left);
    const float height = std::max(ColliderMinSize, bottom - top);
    const Vector2F newCenter(
        left + (width * 0.5f),
        top + (height * 0.5f)
    );

    collider->setSize(width, height);
    collider->setOffset(Vector2F(
        newCenter.x - transformWorldPosition.x,
        newCenter.y - transformWorldPosition.y
    ));
    entityInspector.syncEditStateFromEntity(*entity, true);
}

void LevelEditor::handleViewportEntityInteraction() {
    if (!enabled) {
        draggingEntityId = -1;
        editingColliderEntityId = -1;
        colliderEditMode = ColliderEditMode::None;
        return;
    }

    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload != nullptr) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    if (camera.isPanActive()) {
        draggingEntityId = -1;
        editingColliderEntityId = -1;
        colliderEditMode = ColliderEditMode::None;
        return;
    }

    const ImVec2 mousePosition = ImGui::GetMousePos();
    const RenderRect viewport = camera.getViewport();
    Camera2D& sceneCamera = Renderer::getInstance().getCamera();
    const Vector2F worldMousePosition = sceneCamera.screenToWorld(
        Vector2F(mousePosition.x, mousePosition.y),
        viewport
    );

    if (handleViewportColliderInteraction(worldMousePosition, viewport)) {
        return;
    }

    if (ImGui::IsMouseClicked(ImGuiMouseButton_Left) && !io.WantCaptureMouse) {
        Entity* entity = findEntityAt(worldMousePosition.x, worldMousePosition.y);
        if (entity != nullptr) {
            entityInspector.selectEntity(*entity, true);
            draggingEntityId = entity->getId();
            currentTool = Tool::Move;

            if (TransformComponent* transform = entity->getComponent<TransformComponent>()) {
                const Vector2F& position = transform->getPosition();
                dragOffset[0] = worldMousePosition.x - position.x;
                dragOffset[1] = worldMousePosition.y - position.y;
            } else {
                dragOffset[0] = 0.0f;
                dragOffset[1] = 0.0f;
            }

            statusMessage = "Selected entity " + std::to_string(entity->getId()) + ".";
        } else {
            entityInspector.clearSelection();
            draggingEntityId = -1;
        }
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
        Entity* entity = findEntityById(draggingEntityId);
        if (entity == nullptr) {
            draggingEntityId = -1;
            return;
        }

        TransformComponent* transform = entity->getComponent<TransformComponent>();
        if (transform == nullptr) {
            draggingEntityId = -1;
            return;
        }

        Vector2F newPosition(
            worldMousePosition.x - dragOffset[0],
            worldMousePosition.y - dragOffset[1]
        );
        if (viewportGrid.shouldSnapToGrid()) {
            newPosition = viewportGrid.snapPosition(newPosition);
        }

        transform->setPosition(newPosition);
        entityInspector.syncEditStateFromEntity(*entity, true);
    }

    if (draggingEntityId >= 0 && ImGui::IsMouseReleased(ImGuiMouseButton_Left)) {
        Entity* entity = findEntityById(draggingEntityId);
        if (entity != nullptr) {
            entityInspector.syncEditStateFromEntity(*entity, true);
            statusMessage = "Moved entity " + std::to_string(entity->getId()) + ".";
        }

        draggingEntityId = -1;
    }
}

Entity* LevelEditor::findEntityAt(float x, float y) {
    auto& entities = Level::getCurrentLevel().getEntities();

    for (auto iterator = entities.rbegin(); iterator != entities.rend(); ++iterator) {
        if (iterator->isDestroyed()) {
            continue;
        }

        if (entityContainsPoint(*iterator, x, y)) {
            return &*iterator;
        }
    }

    return nullptr;
}

Entity* LevelEditor::findEntityById(int id) {
    for (Entity& entity : Level::getCurrentLevel().getEntities()) {
        if (entity.getId() == id && !entity.isDestroyed()) {
            return &entity;
        }
    }

    return nullptr;
}

bool LevelEditor::entityContainsPoint(Entity& entity, float x, float y) const {
    TransformComponent* transform = entity.getComponent<TransformComponent>();
    if (transform == nullptr) {
        return false;
    }

    if (viewportGrid.shouldShowColliders()) {
        if (CollisionComponent* collider = entity.getComponent<CollisionComponent>()) {
            Vector2F worldPosition = transform->getWorldPosition();
            const Vector2F& offset = collider->getOffset();
            worldPosition.x += offset.x;
            worldPosition.y += offset.y;
            const float halfWidth = collider->getWidth() * 0.5f;
            const float halfHeight = collider->getHeight() * 0.5f;
            const float left = worldPosition.x - halfWidth;
            const float right = worldPosition.x + halfWidth;
            const float top = worldPosition.y - halfHeight;
            const float bottom = worldPosition.y + halfHeight;

            if (x >= left && x <= right && y >= top && y <= bottom) {
                return true;
            }
        }
    }

    const Sprite* sprite = nullptr;
    if (AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>();
        animationComponent != nullptr && animationComponent->getAnimation().hasFrames()) {
        sprite = &animationComponent->getCurrentFrame();
    } else if (SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
        sprite = &spriteComponent->getSprite();
    }

    if (sprite == nullptr) {
        return false;
    }

    const Vector2F worldPosition = transform->getWorldPosition();
    const Vector2F& size = sprite->getSize();
    const Vector2F& origin = sprite->getOrigin();

    const float left = worldPosition.x - origin.x;
    const float top = worldPosition.y - origin.y;
    const float right = left + size.x;
    const float bottom = top + size.y;

    return x >= left && x <= right && y >= top && y <= bottom;
}
