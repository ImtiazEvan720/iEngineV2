#include "editor/LevelManagerPanel.h"

#include "misc/Level.h"
#include "system/ProjectManager.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <filesystem>

bool LevelManagerPanel::draw(std::string& statusMessage) {
    LevelManager& levelManager = LevelManager::getInstance();
    levelManager.load();

    bool levelLoaded = false;

    if (ImGui::Button("Check For Levels..")) {
        levelManager.refreshFromAssetsFolder();
        statusMessage = "Refreshed level list.";
    }

    ImGui::SameLine();

    if (ImGui::Button("Load Config")) {
        levelManager.reload();
        statusMessage = "Reloaded config.";
    }

    ImGui::SameLine();

    if (ImGui::Button("Save Config")) {
        std::string errorMessage;
        if (levelManager.save(errorMessage)) {
            statusMessage = "Saved level config.";
        } else {
            statusMessage = errorMessage.empty() ? "Failed to save level config." : errorMessage;
        }
    }

    const std::vector<LevelEntry>& levelEntries = levelManager.getLevelEntries();
    ImGui::Text("Levels: %zu", levelEntries.size());
    ImGui::Separator();

    if (levelEntries.empty()) {
        ImGui::TextUnformatted("No .ilevel files found in Assets/Levels.");
        return false;
    }

    constexpr ImGuiTableFlags tableFlags =
        ImGuiTableFlags_Borders |
        ImGuiTableFlags_RowBg |
        ImGuiTableFlags_Resizable |
        ImGuiTableFlags_SizingStretchProp;

    if (ImGui::BeginTable("##LevelManagerTable", 6, tableFlags)) {
        ImGui::TableSetupColumn("Active", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableSetupColumn("File", ImGuiTableColumnFlags_WidthStretch, 1.2f);
        ImGui::TableSetupColumn("Display Name", ImGuiTableColumnFlags_WidthStretch, 1.4f);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch, 2.0f);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 136.0f);
        ImGui::TableSetupColumn("Initial", ImGuiTableColumnFlags_WidthFixed, 64.0f);
        ImGui::TableHeadersRow();

        for (const LevelEntry& entry : levelEntries) {
            if (drawLevelEntry(entry, statusMessage)) {
                levelLoaded = true;
            }
        }

        ImGui::EndTable();
    }

    processPendingRemove(statusMessage);
    return levelLoaded;
}

bool LevelManagerPanel::drawLevelEntry(const LevelEntry& entry, std::string& statusMessage) {
    LevelManager& levelManager = LevelManager::getInstance();
    bool levelLoaded = false;

    ImGui::PushID(entry.fileName.c_str());
    ImGui::TableNextRow();

    ImGui::TableSetColumnIndex(0);
    bool isActive = entry.isActive;
    if (ImGui::Checkbox("##Active", &isActive)) {
        levelManager.setActiveLevel(entry.fileName, isActive);
        statusMessage = isActive
            ? "Set active level: " + entry.fileName
            : "Cleared active level: " + entry.fileName;
    }

    ImGui::TableSetColumnIndex(1);
    ImGui::TextWrapped("%s", entry.fileName.c_str());

    LevelEntry editedEntry = entry;

    ImGui::TableSetColumnIndex(2);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputText("##DisplayName", &editedEntry.displayName)) {
        levelManager.updateLevelEntry(editedEntry);
        statusMessage = "Updated level display name.";
    }

    ImGui::TableSetColumnIndex(3);
    ImGui::SetNextItemWidth(-1.0f);
    if (ImGui::InputTextMultiline("##Description", &editedEntry.description, ImVec2(-1.0f, 48.0f))) {
        levelManager.updateLevelEntry(editedEntry);
        statusMessage = "Updated level description.";
    }

    ImGui::TableSetColumnIndex(4);
    if (ImGui::Button("Load")) {
        levelLoaded = loadLevel(entry, statusMessage);
    }

    ImGui::SameLine();

    if (ImGui::Button("Remove")) {
        pendingRemoveFileName = entry.fileName;
    }

    bool isInitial = levelManager.getInitialLevelPath() == entry.fileName;
    ImGui::TableSetColumnIndex(5);
    if (ImGui::Checkbox("##Initial", &isInitial)) {
        if (isInitial) {
            levelManager.setInitialLevelPath(entry.fileName);
        } else {
            levelManager.setInitialLevelPath("");
        }
    }

    ImGui::PopID();
    return levelLoaded;
}

void LevelManagerPanel::processPendingRemove(std::string& statusMessage) {
    if (pendingRemoveFileName.empty()) {
        return;
    }

    const std::string removedFileName = pendingRemoveFileName;
    pendingRemoveFileName.clear();

    LevelManager::getInstance().removeLevelEntry(removedFileName);
    statusMessage =
        "Removed level entry: " + removedFileName
        + ". The .ilevel file was not deleted.";
}

bool LevelManagerPanel::loadLevel(const LevelEntry& entry, std::string& statusMessage) {
    if (entry.fileName.empty()) {
        statusMessage = "Cannot load level: file name is empty.";
        return false;
    }

    const std::filesystem::path levelPath =
        ProjectManager::getInstance().getAssetsPath() / "Levels" / entry.fileName;

    Level loadedLevel = Level::createEmpty();
    std::string errorMessage;
    if (!Level::loadFromFile(levelPath.string(), loadedLevel, errorMessage)) {
        statusMessage = errorMessage.empty()
            ? "Failed to load level: " + levelPath.string()
            : errorMessage;
        return false;
    }

    Level::loadLevel(std::move(loadedLevel));
    statusMessage = "Loaded level: " + levelPath.string();
    return true;
}
