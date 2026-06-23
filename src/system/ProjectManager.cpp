#include "system/ProjectManager.h"

#include "misc/Level.h"
#include "system/AssetManager.h"
#include "system/EngineState.h"
#include "system/Renderer.h"
#include "system/VirtualInputSystem.h"

#include <tinyxml2.h>

#include <cctype>
#include <iostream>
#include <system_error>
#include <utility>

namespace {
std::filesystem::path normalizePath(const std::filesystem::path& path) {
    if (path.empty()) {
        return path;
    }

    if (path.is_absolute()) {
        return path.lexically_normal();
    }

    return std::filesystem::absolute(path).lexically_normal();
}

const char* getChildPathAttribute(
    const tinyxml2::XMLElement& root,
    const char* childName,
    const char* fallback
) {
    const tinyxml2::XMLElement* child = root.FirstChildElement(childName);
    if (child == nullptr) {
        return fallback;
    }

    const char* path = child->Attribute("path");
    return path == nullptr || path[0] == '\0' ? fallback : path;
}

void appendWarning(std::string& message, const std::string& warning) {
    if (message.empty()) {
        message = warning;
    } else {
        message += " " + warning;
    }
}

bool isValidProjectName(const std::string& projectName) {
    if (projectName.empty()) {
        return false;
    }

    const std::filesystem::path projectNamePath(projectName);
    if (projectNamePath.is_absolute()
        || projectNamePath.has_parent_path()
        || projectNamePath.filename().string() != projectName) {
        return false;
    }

    for (char character : projectName) {
        const unsigned char unsignedCharacter = static_cast<unsigned char>(character);
        if (!(std::isalnum(unsignedCharacter)
              || character == '-'
              || character == '_'
              || character == ' ')) {
            return false;
        }
    }

    return true;
}

bool saveXmlDocument(
    tinyxml2::XMLDocument& document,
    const std::filesystem::path& path,
    std::string& errorMessage
) {
    const tinyxml2::XMLError result = document.SaveFile(path.string().c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to write " + path.string() + ": " + document.ErrorStr();
        return false;
    }

    return true;
}

bool writeProjectFile(
    const std::filesystem::path& projectFilePath,
    const std::string& projectName,
    std::string& errorMessage
) {
    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* project = document.NewElement("project");
    project->SetAttribute("name", projectName.c_str());
    project->SetAttribute("version", 1);
    document.InsertEndChild(project);

    tinyxml2::XMLElement* config = document.NewElement("config");
    config->SetAttribute("path", "config.xml");
    project->InsertEndChild(config);

    tinyxml2::XMLElement* assets = document.NewElement("assets");
    assets->SetAttribute("path", "Assets");
    project->InsertEndChild(assets);

    tinyxml2::XMLElement* startupLevel = document.NewElement("startupLevel");
    startupLevel->SetAttribute("path", "Assets/Levels/current.ilevel");
    project->InsertEndChild(startupLevel);

    return saveXmlDocument(document, projectFilePath, errorMessage);
}

bool writeDefaultConfig(const std::filesystem::path& configPath, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* config = document.NewElement("config");
    document.InsertEndChild(config);

    tinyxml2::XMLElement* window = document.NewElement("window");
    window->SetAttribute("title", "iEngine(Alpha)");
    window->SetAttribute("width", 1280);
    window->SetAttribute("height", 720);
    window->SetAttribute("framerate", 60);
    config->InsertEndChild(window);

    tinyxml2::XMLElement* graphics = document.NewElement("graphics");
    graphics->SetAttribute("clearColorR", 45);
    graphics->SetAttribute("clearColorG", 45);
    graphics->SetAttribute("clearColorB", 50);
    config->InsertEndChild(graphics);

    return saveXmlDocument(document, configPath, errorMessage);
}

bool writeDefaultInputBindings(const std::filesystem::path& inputPath, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* input = document.NewElement("input");
    document.InsertEndChild(input);

    tinyxml2::XMLElement* bindings = document.NewElement("bindings");
    input->InsertEndChild(bindings);

    auto addBinding = [&](const char* action, const char* attributeName, const char* value) {
        tinyxml2::XMLElement* binding = document.NewElement("binding");
        binding->SetAttribute("action", action);
        binding->SetAttribute(attributeName, value);
        bindings->InsertEndChild(binding);
    };

    addBinding("MoveUp", "key", "W");
    addBinding("MoveDown", "key", "S");
    addBinding("MoveLeft", "key", "A");
    addBinding("MoveRight", "key", "D");
    addBinding("Fire", "key", "Space");
    addBinding("Fire2", "key", "Escape");
    addBinding("Fire", "mouse", "Left");
    addBinding("MoveUp", "touchControl", "MoveStickUp");
    addBinding("MoveDown", "touchControl", "MoveStickDown");
    addBinding("MoveLeft", "touchControl", "MoveStickLeft");
    addBinding("MoveRight", "touchControl", "MoveStickRight");
    addBinding("Fire", "touchControl", "FireButton");

    return saveXmlDocument(document, inputPath, errorMessage);
}

bool writeEmptyLevel(const std::filesystem::path& levelPath, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* level = document.NewElement("level");
    level->SetAttribute("version", 1);
    document.InsertEndChild(level);

    tinyxml2::XMLElement* entities = document.NewElement("entities");
    entities->SetAttribute("count", 0);
    level->InsertEndChild(entities);

    return saveXmlDocument(document, levelPath, errorMessage);
}
}

ProjectManager& ProjectManager::getInstance() {
    static ProjectManager instance;
    return instance;
}

void ProjectManager::setDefaultProjectRoot(const std::filesystem::path& projectRoot) {
    std::filesystem::path root = projectRoot.empty()
        ? std::filesystem::current_path()
        : projectRoot;
    root = normalizePath(root);

    currentProject.projectRoot = root;
    currentProject.projectFilePath = root / "project.iengine";
    currentProject.name = root.filename().empty() ? "iEngine Project" : root.filename().string();
    currentProject.configPath = root / "config.xml";
    currentProject.assetsPath = root / "Assets";
    currentProject.startupLevelPath = root / "Assets" / "Levels" / "current.ilevel";
}

bool ProjectManager::createProject(
    const std::filesystem::path& parentDirectory,
    const std::string& projectName,
    std::string& errorMessage
) {
    namespace fs = std::filesystem;

    if (!isValidProjectName(projectName)) {
        errorMessage =
            "Project name can only contain letters, numbers, spaces, hyphens, and underscores.";
        return false;
    }

    const fs::path parent = normalizePath(parentDirectory.empty()
        ? fs::current_path()
        : parentDirectory);
    if (!fs::exists(parent) || !fs::is_directory(parent)) {
        errorMessage = "Project parent directory does not exist: " + parent.string();
        return false;
    }

    const fs::path projectRoot = parent / projectName;
    std::error_code errorCode;
    if (fs::exists(projectRoot, errorCode)
        && fs::is_directory(projectRoot, errorCode)
        && !fs::is_empty(projectRoot, errorCode)) {
        errorMessage = "Project directory already exists and is not empty: "
            + projectRoot.string();
        return false;
    }

    fs::create_directories(projectRoot / "Assets" / "Input", errorCode);
    if (errorCode) {
        errorMessage = "Failed to create project input directory: " + errorCode.message();
        return false;
    }

    fs::create_directories(projectRoot / "Assets" / "Levels", errorCode);
    if (errorCode) {
        errorMessage = "Failed to create project levels directory: " + errorCode.message();
        return false;
    }

    fs::create_directories(projectRoot / "Assets" / "Prefabs", errorCode);
    if (errorCode) {
        errorMessage = "Failed to create project prefabs directory: " + errorCode.message();
        return false;
    }

    fs::create_directories(projectRoot / "Assets" / "Scripts", errorCode);
    if (errorCode) {
        errorMessage = "Failed to create project scripts directory: " + errorCode.message();
        return false;
    }

    const fs::path projectFilePath = projectRoot / "project.iengine";
    if (!writeProjectFile(projectFilePath, projectName, errorMessage)) {
        return false;
    }

    if (!writeDefaultConfig(projectRoot / "config.xml", errorMessage)) {
        return false;
    }

    if (!writeDefaultInputBindings(projectRoot / "Assets" / "Input" / "default.input.xml", errorMessage)) {
        return false;
    }

    if (!writeEmptyLevel(projectRoot / "Assets" / "Levels" / "current.ilevel", errorMessage)) {
        return false;
    }

    std::string openWarning;
    if (!openProject(projectFilePath.string(), openWarning)) {
        errorMessage = openWarning.empty()
            ? "Created project, but failed to open it."
            : openWarning;
        return false;
    }

    errorMessage = openWarning;
    return true;
}

bool ProjectManager::openProject(const std::string& projectFilePath, std::string& errorMessage) {
    ProjectInfo nextProject;
    if (!parseProjectFile(projectFilePath, nextProject, errorMessage)) {
        return false;
    }

    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Edit);
    Level::loadLevel(Level::createEmpty());
    Renderer::getInstance().clearTileLayerBatches();
    AssetManager::getInstance().clearAssets();

    currentProject = std::move(nextProject);

    if (!AssetManager::getInstance().loadAssets(currentProject.assetsPath.string())) {
        appendWarning(errorMessage, "Opened project, but one or more assets failed to load.");
    }

    VirtualInputSystem& virtualInputSystem = VirtualInputSystem::getInstance();
    const std::filesystem::path inputBindingsPath =
        currentProject.assetsPath / "Input" / "default.input.xml";
    std::string inputError;
    if (!virtualInputSystem.loadBindingsFromFile(inputBindingsPath.string(), inputError)) {
        virtualInputSystem.bindDefaultKeyboardMouse();
        appendWarning(errorMessage, inputError + " Falling back to default input bindings.");
    }

    Level startupLevel = Level::createEmpty();
    std::string levelError;
    if (Level::loadFromFile(currentProject.startupLevelPath.string(), startupLevel, levelError)) {
        Level::loadLevel(std::move(startupLevel));
    } else {
        Level::loadLevel(Level::createEmpty());
        Level::setCurrentLevelPath(currentProject.startupLevelPath.string());
        appendWarning(
            errorMessage,
            "Opened project, but failed to load startup level: " + levelError
        );
    }
    return true;
}

void ProjectManager::closeProject() {
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Edit);
    Level::loadLevel(Level::createEmpty());
    Renderer::getInstance().clearTileLayerBatches();
    AssetManager::getInstance().clearAssets();
    currentProject = ProjectInfo{};
}

const ProjectInfo& ProjectManager::getCurrentProject() const {
    return currentProject;
}

const std::filesystem::path& ProjectManager::getProjectRoot() const {
    return currentProject.projectRoot;
}

const std::filesystem::path& ProjectManager::getAssetsPath() const {
    return currentProject.assetsPath;
}

const std::filesystem::path& ProjectManager::getConfigPath() const {
    return currentProject.configPath;
}

const std::filesystem::path& ProjectManager::getStartupLevelPath() const {
    return currentProject.startupLevelPath;
}

std::filesystem::path ProjectManager::resolveProjectPath(
    const std::filesystem::path& projectRoot,
    const std::string& path
) {
    std::filesystem::path resolved(path);
    if (resolved.is_absolute()) {
        return resolved.lexically_normal();
    }

    return (projectRoot / resolved).lexically_normal();
}

bool ProjectManager::parseProjectFile(
    const std::filesystem::path& projectFilePath,
    ProjectInfo& projectInfo,
    std::string& errorMessage
) {
    const std::filesystem::path normalizedProjectFile = normalizePath(projectFilePath);
    if (!std::filesystem::exists(normalizedProjectFile)) {
        errorMessage = "Project file does not exist: " + normalizedProjectFile.string();
        return false;
    }

    tinyxml2::XMLDocument document;
    if (document.LoadFile(normalizedProjectFile.string().c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load project file: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* root = document.FirstChildElement("project");
    if (root == nullptr) {
        errorMessage = "Project file is missing project root.";
        return false;
    }

    const std::filesystem::path projectRoot = normalizedProjectFile.parent_path();
    const char* projectName = root->Attribute("name");
    const char* configPath = getChildPathAttribute(*root, "config", "config.xml");
    const char* assetsPath = getChildPathAttribute(*root, "assets", "Assets");
    const char* startupLevelPath = getChildPathAttribute(
        *root,
        "startupLevel",
        "Assets/Levels/current.ilevel"
    );

    projectInfo.projectFilePath = normalizedProjectFile;
    projectInfo.projectRoot = projectRoot;
    projectInfo.name = projectName == nullptr || projectName[0] == '\0'
        ? normalizedProjectFile.stem().string()
        : projectName;
    projectInfo.configPath = resolveProjectPath(projectRoot, configPath);
    projectInfo.assetsPath = resolveProjectPath(projectRoot, assetsPath);
    projectInfo.startupLevelPath = resolveProjectPath(projectRoot, startupLevelPath);

    if (!std::filesystem::exists(projectInfo.assetsPath)
        || !std::filesystem::is_directory(projectInfo.assetsPath)) {
        errorMessage = "Project assets directory does not exist: "
            + projectInfo.assetsPath.string();
        return false;
    }

    errorMessage.clear();
    return true;
}
