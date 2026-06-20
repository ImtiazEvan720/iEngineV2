#include "editor/LevelEditor.h"

#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>
#include <vector>

namespace {
struct ProjectBrowserEntry {
    std::filesystem::path path;
    bool directory = false;
};

std::vector<ProjectBrowserEntry> getProjectBrowserEntries(const std::filesystem::path& directory) {
    namespace fs = std::filesystem;

    std::vector<ProjectBrowserEntry> entries;
    std::error_code errorCode;
    if (!fs::exists(directory, errorCode) || !fs::is_directory(directory, errorCode)) {
        return entries;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(directory, errorCode)) {
        if (errorCode) {
            break;
        }

        const bool isDirectory = entry.is_directory(errorCode);
        if (errorCode) {
            errorCode.clear();
            continue;
        }

        if (!isDirectory && entry.path().extension() != ".iengine") {
            continue;
        }

        entries.push_back({entry.path(), isDirectory});
    }

    std::sort(
        entries.begin(),
        entries.end(),
        [](const ProjectBrowserEntry& left, const ProjectBrowserEntry& right) {
            if (left.directory != right.directory) {
                return left.directory;
            }

            return left.path.filename().string() < right.path.filename().string();
        }
    );

    return entries;
}

bool isProjectFile(const std::filesystem::path& path) {
    return path.extension() == ".iengine";
}
}

void LevelEditor::drawProjectMenu() {
    if (!ImGui::BeginMenu("Project")) {
        return;
    }

    const ProjectInfo& project = ProjectManager::getInstance().getCurrentProject();
    ImGui::TextDisabled(
        "Current: %s",
        project.name.empty() ? "Default runtime project" : project.name.c_str()
    );

    if (ImGui::MenuItem("Open...")) {
        openProjectOpenWindow();
    }

    if (ImGui::MenuItem("New...")) {
        openProjectNewWindow();
    }

    ImGui::EndMenu();
}

void LevelEditor::openProjectNewWindow() {
    const std::filesystem::path projectRoot = ProjectManager::getInstance().getProjectRoot();
    if (!projectRoot.empty() && !projectRoot.parent_path().empty()) {
        newProjectParentPath = projectRoot.parent_path().string();
    } else {
        newProjectParentPath = std::filesystem::current_path().string();
    }

    if (newProjectName.empty()) {
        newProjectName = "NewProject";
    }

    showProjectNewWindow = true;
}

void LevelEditor::drawProjectNewWindow() {
    if (!showProjectNewWindow) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("New Project", &showProjectNewWindow)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Create project");
    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("Parent Folder", &newProjectParentPath);

    std::filesystem::path parentPath(newProjectParentPath);
    if (ImGui::Button("Up")) {
        const std::filesystem::path parent = parentPath.parent_path();
        if (!parent.empty()) {
            newProjectParentPath = parent.string();
            parentPath = parent;
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Use Current Project Parent")) {
        const std::filesystem::path projectRoot = ProjectManager::getInstance().getProjectRoot();
        if (!projectRoot.empty() && !projectRoot.parent_path().empty()) {
            newProjectParentPath = projectRoot.parent_path().string();
            parentPath = newProjectParentPath;
        }
    }

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("Project Name", &newProjectName);

    if (ImGui::BeginChild(
            "##NewProjectParentFolders",
            ImVec2(0.0f, -112.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        const std::vector<ProjectBrowserEntry> entries = getProjectBrowserEntries(parentPath);
        bool foundFolder = false;

        for (const ProjectBrowserEntry& entry : entries) {
            if (!entry.directory) {
                continue;
            }

            foundFolder = true;
            const std::string label = "[Folder] " + entry.path.filename().string();
            if (ImGui::Selectable(label.c_str())) {
                newProjectParentPath = entry.path.string();
            }
        }

        if (!foundFolder) {
            ImGui::TextDisabled("No folders found here.");
        }
    }
    ImGui::EndChild();

    const std::filesystem::path projectRoot =
        std::filesystem::path(newProjectParentPath) / newProjectName;
    ImGui::TextWrapped("Project path: %s", projectRoot.string().c_str());

    if (ImGui::Button("Create")) {
        createNewProject();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        showProjectNewWindow = false;
    }

    if (!statusMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::End();
}

void LevelEditor::openProjectOpenWindow() {
    const std::filesystem::path projectRoot = ProjectManager::getInstance().getProjectRoot();
    if (!projectRoot.empty()) {
        projectBrowserPath = projectRoot.string();
    } else {
        projectBrowserPath = std::filesystem::current_path().string();
    }

    selectedProjectPath.clear();
    showProjectOpenWindow = true;
}

void LevelEditor::drawProjectOpenWindow() {
    if (!showProjectOpenWindow) {
        return;
    }

    ImGui::SetNextWindowSize(ImVec2(620.0f, 420.0f), ImGuiCond_FirstUseEver);
    if (!ImGui::Begin("Open Project", &showProjectOpenWindow)) {
        ImGui::End();
        return;
    }

    ImGui::TextUnformatted("Project file");
    ImGui::Separator();

    ImGui::SetNextItemWidth(-1.0f);
    ImGui::InputText("##ProjectBrowserPath", &projectBrowserPath);

    std::filesystem::path browserPath(projectBrowserPath);
    if (isProjectFile(browserPath)) {
        if (ImGui::Button("Open Path")) {
            selectedProjectPath = browserPath.string();
            openSelectedProject();
        }
    } else {
        if (ImGui::Button("Up")) {
            const std::filesystem::path parent = browserPath.parent_path();
            if (!parent.empty()) {
                projectBrowserPath = parent.string();
                selectedProjectPath.clear();
            }
        }
    }

    ImGui::SameLine();
    if (ImGui::Button("Refresh")) {
        selectedProjectPath.clear();
    }

    ImGui::Separator();

    if (ImGui::BeginChild(
            "##ProjectBrowserEntries",
            ImVec2(0.0f, -64.0f),
            ImGuiChildFlags_Borders,
            ImGuiWindowFlags_HorizontalScrollbar)) {
        const std::vector<ProjectBrowserEntry> entries = getProjectBrowserEntries(browserPath);
        if (entries.empty()) {
            ImGui::TextDisabled("No folders or .iengine files found here.");
        }

        for (const ProjectBrowserEntry& entry : entries) {
            const std::string label =
                std::string(entry.directory ? "[Folder] " : "[Project] ")
                + entry.path.filename().string();
            const bool selected = selectedProjectPath == entry.path.string();

            if (ImGui::Selectable(label.c_str(), selected)) {
                if (entry.directory) {
                    selectedProjectPath.clear();
                } else {
                    selectedProjectPath = entry.path.string();
                }
            }

            if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
                if (entry.directory) {
                    projectBrowserPath = entry.path.string();
                    selectedProjectPath.clear();
                } else {
                    selectedProjectPath = entry.path.string();
                    openSelectedProject();
                }
            }
        }
    }
    ImGui::EndChild();

    const bool hasSelectedProject = !selectedProjectPath.empty();
    if (!hasSelectedProject) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Open Selected")) {
        openSelectedProject();
    }

    if (!hasSelectedProject) {
        ImGui::EndDisabled();
    }

    ImGui::SameLine();
    if (ImGui::Button("Cancel")) {
        showProjectOpenWindow = false;
    }

    if (!selectedProjectPath.empty()) {
        ImGui::TextWrapped("Selected: %s", selectedProjectPath.c_str());
    }

    if (!statusMessage.empty()) {
        ImGui::Separator();
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::End();
}

void LevelEditor::createNewProject() {
    std::string errorMessage;
    if (!ProjectManager::getInstance().createProject(
            newProjectParentPath,
            newProjectName,
            errorMessage)) {
        statusMessage = errorMessage.empty() ? "Failed to create project." : errorMessage;
        return;
    }

    Renderer::getInstance().clearTileLayerBatches();
    entityInspector.clearSelection();
    spritePalette.refreshTilesets();
    prefabPanel.refreshPrefabs();

    const ProjectInfo& project = ProjectManager::getInstance().getCurrentProject();
    statusMessage = "Created project: " + project.name;
    if (!errorMessage.empty()) {
        statusMessage += ". " + errorMessage;
    }

    projectBrowserPath = project.projectRoot.string();
    showProjectNewWindow = false;
}

void LevelEditor::openSelectedProject() {
    if (selectedProjectPath.empty()) {
        statusMessage = "Select a .iengine project file first.";
        return;
    }

    std::string errorMessage;
    if (!ProjectManager::getInstance().openProject(selectedProjectPath, errorMessage)) {
        statusMessage = errorMessage.empty() ? "Failed to open project." : errorMessage;
        return;
    }

    Renderer::getInstance().clearTileLayerBatches();
    entityInspector.clearSelection();
    spritePalette.refreshTilesets();
    prefabPanel.refreshPrefabs();

    const ProjectInfo& project = ProjectManager::getInstance().getCurrentProject();
    statusMessage = "Opened project: " + project.name;
    if (!errorMessage.empty()) {
        statusMessage += ". " + errorMessage;
    }

    projectBrowserPath = project.projectRoot.string();
    showProjectOpenWindow = false;
}
