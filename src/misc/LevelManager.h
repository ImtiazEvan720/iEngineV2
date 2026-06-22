#pragma once

#include <filesystem>
#include <string>
#include <vector>

struct LevelEntry
{
    std::string fileName;
    std::string displayName;
    std::string description;
    bool isActive = false;
};

class LevelManager
{
public:
    static LevelManager &getInstance();

    void load();
    void reload();
    bool save(std::string& errorMessage);
    void refreshFromAssetsFolder();

    const std::vector<LevelEntry> &getLevelEntries() const;
    void addLevelEntry(const LevelEntry &entry);
    void removeLevelEntry(const LevelEntry &entry);
    void removeLevelEntry(const std::string &fileName);

    bool updateLevelEntry(const LevelEntry& entry);
    bool setActiveLevel(const std::string& fileName, bool value);
    const std::vector<const LevelEntry*> getActiveLevelEntries() const;
    bool resolveLevelPath(
        const std::string& levelName,
        std::filesystem::path& levelPath,
        std::string& errorMessage
    );

    const std::string& getInitialLevelPath() const;
    void setInitialLevelPath(const std::string& path);

private:
    inline static constexpr const char* DefaultInitialLevelPath = "current.ilevel";

    std::vector<LevelEntry> levelEntries;
    bool loaded = false;
    std::string initialLevelPath = DefaultInitialLevelPath;

    LevelManager() = default;
};
