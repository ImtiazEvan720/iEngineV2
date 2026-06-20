#include "editor/LevelEditor.h"

#include "system/ProjectManager.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <vector>

std::vector<std::filesystem::path> LevelEditor::getLevelFiles() const {
    namespace fs = std::filesystem;

    std::vector<fs::path> levelFiles;
    const fs::path levelsDirectory = ProjectManager::getInstance().getAssetsPath() / "Levels";
    if (!fs::exists(levelsDirectory) || !fs::is_directory(levelsDirectory)) {
        return levelFiles;
    }

    for (const fs::directory_entry& entry : fs::directory_iterator(levelsDirectory)) {
        if (!entry.is_regular_file()) {
            continue;
        }

        const fs::path path = entry.path();
        if (path.extension() == ".ilevel") {
            levelFiles.push_back(path);
        }
    }

    std::sort(levelFiles.begin(), levelFiles.end());
    return levelFiles;
}

void LevelEditor::drawFileExplorerTab() {
    ImGui::Text("Levels");
    ImGui::Separator();

    const std::vector<std::filesystem::path> levelFiles = getLevelFiles();
    if (levelFiles.empty()) {
        const std::filesystem::path levelsPath =
            ProjectManager::getInstance().getAssetsPath() / "Levels";
        ImGui::TextDisabled("No .ilevel files found in %s.", levelsPath.string().c_str());
        return;
    }

    for (const std::filesystem::path& path : levelFiles) {
        if (ImGui::Selectable(path.filename().string().c_str())) {
            loadSelectedLevel(path.string());
        }
    }
}
