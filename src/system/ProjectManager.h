#ifndef IENGINEV2_PROJECTMANAGER_H
#define IENGINEV2_PROJECTMANAGER_H

#include <filesystem>
#include <string>

struct ProjectInfo {
    std::string name;
    std::filesystem::path projectFilePath;
    std::filesystem::path projectRoot;
    std::filesystem::path configPath;
    std::filesystem::path assetsPath;
    std::filesystem::path startupLevelPath;
};

class ProjectManager {
public:
    static ProjectManager& getInstance();

    ProjectManager(const ProjectManager& other) = delete;
    ProjectManager& operator=(const ProjectManager& other) = delete;
    ProjectManager(ProjectManager&& other) = delete;
    ProjectManager& operator=(ProjectManager&& other) = delete;

    void setDefaultProjectRoot(const std::filesystem::path& projectRoot);
    bool createProject(
        const std::filesystem::path& parentDirectory,
        const std::string& projectName,
        std::string& errorMessage
    );
    bool openProject(const std::string& projectFilePath, std::string& errorMessage);
    void closeProject();

    const ProjectInfo& getCurrentProject() const;
    const std::filesystem::path& getProjectRoot() const;
    const std::filesystem::path& getAssetsPath() const;
    const std::filesystem::path& getConfigPath() const;
    const std::filesystem::path& getStartupLevelPath() const;

private:
    ProjectManager() = default;

    static std::filesystem::path resolveProjectPath(
        const std::filesystem::path& projectRoot,
        const std::string& path
    );
    static bool parseProjectFile(
        const std::filesystem::path& projectFilePath,
        ProjectInfo& projectInfo,
        std::string& errorMessage
    );

    ProjectInfo currentProject;
};

#endif
