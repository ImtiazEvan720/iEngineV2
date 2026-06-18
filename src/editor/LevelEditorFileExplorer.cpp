#include "editor/LevelEditor.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>
#include <vector>

std::vector<std::filesystem::path> LevelEditor::getLevelFiles() const {
    namespace fs = std::filesystem;

    std::vector<fs::path> levelFiles;
    const fs::path levelsDirectory("Assets/Levels");
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
        ImGui::TextDisabled("No .ilevel files found in Assets/Levels.");
        return;
    }

    for (const std::filesystem::path& path : levelFiles) {
        if (ImGui::Selectable(path.filename().string().c_str())) {
            loadSelectedLevel(path.string());
        }
    }
}
