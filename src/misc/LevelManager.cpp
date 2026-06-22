#include "misc/LevelManager.h"

#include "system/ProjectManager.h"

#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <system_error>

namespace
{

    const std::string configFileName = "levels.xml";

    std::filesystem::path getLevelsDirectory()
    {
        return ProjectManager::getInstance().getAssetsPath() / "Levels";
    }

    std::filesystem::path getLevelConfigPath()
    {
        return getLevelsDirectory() / configFileName;
    }

    bool isLevelFile(const std::filesystem::directory_entry &entry)
    {
        return entry.is_regular_file() && entry.path().extension() == ".ilevel";
    }

    bool containsFileName(const std::vector<std::string> &fileNames, const std::string &fileName)
    {
        return std::find(fileNames.begin(), fileNames.end(), fileName) != fileNames.end();
    }

    bool matchesLevelName(const LevelEntry& entry, const std::string& levelName)
    {
        if (entry.fileName == levelName || entry.displayName == levelName)
        {
            return true;
        }

        const std::filesystem::path fileNamePath(entry.fileName);
        return fileNamePath.stem().string() == levelName;
    }

    bool isSafeLevelFileName(const std::string& levelName)
    {
        if (levelName.empty())
        {
            return false;
        }

        const std::filesystem::path levelPath(levelName);
        return !levelPath.is_absolute()
            && !levelPath.has_parent_path()
            && levelPath.filename().string() == levelName;
    }

    LevelEntry makeDefaultEntry(const std::filesystem::path &levelPath)
    {
        LevelEntry entry;
        entry.fileName = levelPath.filename().string();
        entry.displayName = levelPath.stem().string();
        entry.description = "";
        entry.isActive = false;
        return entry;
    }

    const tinyxml2::XMLElement *getDescriptionElement(const tinyxml2::XMLElement &levelElement)
    {
        return levelElement.FirstChildElement("description");
    }

}

LevelManager &LevelManager::getInstance()
{
    static LevelManager instance;
    return instance;
}

void LevelManager::load()
{
    if (loaded)
    {
        return;
    }

    levelEntries.clear();
    initialLevelPath = DefaultInitialLevelPath;

    const std::filesystem::path configPath = getLevelConfigPath();
    if (std::filesystem::exists(configPath))
    {
        tinyxml2::XMLDocument document;
        if (document.LoadFile(configPath.string().c_str()) == tinyxml2::XML_SUCCESS)
        {
            const tinyxml2::XMLElement *levelsElement = document.FirstChildElement("levels");

            for (const tinyxml2::XMLElement *levelElement =
                     levelsElement == nullptr ? nullptr : levelsElement->FirstChildElement("level");
                 levelElement != nullptr;
                 levelElement = levelElement->NextSiblingElement("level"))
            {
                const char *fileName = levelElement->Attribute("fileName");
                if (fileName == nullptr || fileName[0] == '\0')
                {
                    continue;
                }

                LevelEntry entry;
                entry.fileName = fileName;

                const char *displayName = levelElement->Attribute("displayName");
                entry.displayName = displayName == nullptr || displayName[0] == '\0'
                                        ? std::filesystem::path(entry.fileName).stem().string()
                                        : displayName;

                const tinyxml2::XMLElement *description = getDescriptionElement(*levelElement);
                const char *descriptionText = description == nullptr ? nullptr : description->GetText();
                entry.description = descriptionText == nullptr ? "" : descriptionText;
                entry.isActive = levelElement->BoolAttribute("active", false);

                addLevelEntry(entry);
            }

            const tinyxml2::XMLElement *initialLevelElement = levelsElement->FirstChildElement("initialLevel");
            if (initialLevelElement != nullptr)
            {
                const char *initialLevelPath = initialLevelElement->Attribute("path");
                if (initialLevelPath != nullptr)
                {
                    setInitialLevelPath(initialLevelPath);
                }
            }
        }
    }
    
    loaded = true;
}

void LevelManager::reload()
{
    loaded = false;
    load();
}

bool LevelManager::save(std::string &errorMessage)
{
    if (!loaded)
    {
        load();
    }

    const std::filesystem::path levelsDirectory = getLevelsDirectory();
    std::error_code directoryError;
    std::filesystem::create_directories(levelsDirectory, directoryError);
    if (directoryError)
    {
        errorMessage = "Failed to create levels directory: " + directoryError.message();
        return false;
    }

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement *levelsElement = document.NewElement("levels");
    levelsElement->SetAttribute("version", 1);
    levelsElement->SetAttribute("count", static_cast<int>(levelEntries.size()));
    document.InsertEndChild(levelsElement);

    for (const LevelEntry &entry : levelEntries)
    {
        if (entry.fileName.empty())
        {
            continue;
        }

        tinyxml2::XMLElement *levelElement = document.NewElement("level");
        levelElement->SetAttribute("fileName", entry.fileName.c_str());
        levelElement->SetAttribute("displayName", entry.displayName.c_str());
        levelElement->SetAttribute("active", entry.isActive ? "true" : "false");
        levelsElement->InsertEndChild(levelElement);

        tinyxml2::XMLElement *descriptionElement = document.NewElement("description");
        descriptionElement->SetText(entry.description.c_str());
        levelElement->InsertEndChild(descriptionElement);
    }

    if (!initialLevelPath.empty())
    {
        tinyxml2::XMLElement *initialLevelElement = document.NewElement("initialLevel");
        initialLevelElement->SetAttribute("path", initialLevelPath.c_str());
        levelsElement->InsertEndChild(initialLevelElement);
    }

    const std::filesystem::path configPath = getLevelConfigPath();
    const tinyxml2::XMLError result = document.SaveFile(configPath.string().c_str());
    if (result != tinyxml2::XML_SUCCESS)
    {
        errorMessage = "Failed to save level config: " + std::string(document.ErrorStr());
        return false;
    }

    errorMessage.clear();
    return true;
}

void LevelManager::refreshFromAssetsFolder()
{
    const std::filesystem::path levelsDirectory = getLevelsDirectory();
    std::vector<std::string> discoveredFileNames;

    if (std::filesystem::exists(levelsDirectory))
    {
        try
        {
            for (const std::filesystem::directory_entry &entry :
                 std::filesystem::directory_iterator(levelsDirectory))
            {
                if (isLevelFile(entry))
                {
                    discoveredFileNames.push_back(entry.path().filename().string());
                }
            }
        }
        catch (const std::filesystem::filesystem_error &)
        {
            discoveredFileNames.clear();
        }
    }

    std::sort(discoveredFileNames.begin(), discoveredFileNames.end());

    levelEntries.erase(
        std::remove_if(
            levelEntries.begin(),
            levelEntries.end(),
            [&discoveredFileNames](const LevelEntry &entry)
            {
                return !containsFileName(discoveredFileNames, entry.fileName);
            }),
        levelEntries.end());

    for (const std::string &fileName : discoveredFileNames)
    {
        auto matchesFileName = [&fileName](const LevelEntry &entry)
        {
            return entry.fileName == fileName;
        };

        const auto matchingEntry = std::find_if(
            levelEntries.begin(),
            levelEntries.end(),
            matchesFileName);

        if (matchingEntry == levelEntries.end())
        {
            levelEntries.push_back(makeDefaultEntry(levelsDirectory / fileName));
        }
    }

    loaded = true;
}

const std::vector<LevelEntry> &LevelManager::getLevelEntries() const
{
    return levelEntries;
}

void LevelManager::addLevelEntry(const LevelEntry &entry)
{
    levelEntries.push_back(entry);
}

void LevelManager::removeLevelEntry(const LevelEntry &entry)
{
    removeLevelEntry(entry.fileName);
}

void LevelManager::removeLevelEntry(const std::string &fileName)
{

    auto matchesFileName = [&fileName](const LevelEntry &entry)
    { return entry.fileName == fileName; };

    // Move matching entries to the end and return where the kept range ends.
    auto removeStart = std::remove_if(
        levelEntries.begin(),
        levelEntries.end(),
        matchesFileName);

    // Actually erase the removed range from the vector.
    levelEntries.erase(removeStart, levelEntries.end());
}

bool LevelManager::updateLevelEntry(const LevelEntry &entry)
{
    for (LevelEntry &existingEntry : levelEntries)
    {
        if (existingEntry.fileName == entry.fileName)
        {
            existingEntry = entry;
            return true;
        }
    }

    return false;
}

bool LevelManager::setActiveLevel(const std::string &fileName, bool value)
{

    for (LevelEntry &entry : levelEntries)
    {
        if (entry.fileName == fileName)
        {
            entry.isActive = value;
            return true;
        }
    }
    return false;
}

const std::vector<const LevelEntry *> LevelManager::getActiveLevelEntries() const
{
    std::vector<const LevelEntry *> activeEntries;

    for (const LevelEntry &entry : levelEntries)
    {
        if (entry.isActive)
        {
            activeEntries.push_back(&entry);
        }
    }

    return activeEntries;
}

bool LevelManager::resolveLevelPath(
    const std::string& levelName,
    std::filesystem::path& levelPath,
    std::string& errorMessage
)
{
    if (levelName.empty())
    {
        errorMessage = "Level name is empty.";
        return false;
    }

    load();

    if (levelEntries.empty())
    {
        refreshFromAssetsFolder();
    }

    const std::filesystem::path levelsDirectory = getLevelsDirectory();
    for (const LevelEntry& entry : levelEntries)
    {
        if (matchesLevelName(entry, levelName))
        {
            levelPath = levelsDirectory / entry.fileName;
            if (!std::filesystem::exists(levelPath))
            {
                errorMessage = "Level file does not exist: " + levelPath.string();
                return false;
            }

            errorMessage.clear();
            return true;
        }
    }

    if (!isSafeLevelFileName(levelName))
    {
        errorMessage = "Level name cannot contain a path: " + levelName;
        return false;
    }

    std::filesystem::path fileName(levelName);
    if (fileName.extension() != ".ilevel")
    {
        fileName += ".ilevel";
    }

    const std::filesystem::path fallbackPath = levelsDirectory / fileName;
    if (std::filesystem::exists(fallbackPath))
    {
        levelPath = fallbackPath;
        errorMessage.clear();
        return true;
    }

    errorMessage = "Level not found: " + levelName;
    return false;
}

const std::string& LevelManager::getInitialLevelPath() const
{
    return initialLevelPath;
}

void LevelManager::setInitialLevelPath(const std::string& path)
{
    initialLevelPath = path;
}
