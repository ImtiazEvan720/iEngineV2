#include "Application.h"

#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/RenderConstants.h"
#include "misc/LevelManager.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/EngineState.h"
#include "system/IGuiBackend.h"
#include "system/IRenderBackend.h"
#include "system/IWindowBackend.h"
#include "system/InputSystem.h"
#include "system/PhysicsSystem.h"
#include "system/ProjectManager.h"
#include "system/RawInputSystem.h"
#include "system/Renderer.h"
#include "system/ScriptSystem.h"
#include "system/sdl/SdlGuiBackend.h"
#include "system/sdl/SdlRenderBackend.h"
#include "system/sdl/SdlWindowBackend.h"
#include "system/TouchControlSystem.h"
#include "system/UISystem.h"
#include "system/VirtualInputSystem.h"

#ifndef IENGINE_SDL_ONLY
#include "system/sfml/SfmlGuiBackend.h"
#include "system/sfml/SfmlRenderBackend.h"
#include "system/sfml/SfmlWindowBackend.h"
#endif

#include <box2d/box2d.h>
#include <tinyxml2.h>

#ifdef IENGINE_ANDROID
#include <SDL3/SDL_system.h>
#endif

#ifdef __EMSCRIPTEN__
#include <emscripten/emscripten.h>
#endif

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

#ifdef __EMSCRIPTEN__
EM_JS(bool, iengineIsTouchDevice, (), {
    return (navigator.maxTouchPoints && navigator.maxTouchPoints > 0) ||
        (window.matchMedia && window.matchMedia("(pointer: coarse)").matches);
});
#else
bool iengineIsTouchDevice() {
    return false;
}
#endif

namespace {
std::string getBackendName(int argc, char* argv[]) {
#ifdef IENGINE_SDL_ONLY
    std::string backendName = "sdl";
#else
    std::string backendName = "sfml";
#endif

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];
        if (argument == "--sdl") {
            backendName = "sdl";
        } else if (argument == "--sfml") {
            backendName = "sfml";
        } else if (argument.rfind("--backend=", 0) == 0) {
            backendName = argument.substr(std::string("--backend=").size());
        }
    }

    return backendName;
}

std::string getDataRootOverride(int argc, char* argv[]) {
    std::string overrideValue;

    for (int i = 1; i < argc; ++i) {
        const std::string argument = argv[i];

        if (argument == "--data-root" && i + 1 < argc) {
            overrideValue = argv[i + 1];
            ++i;
        } else if (argument.rfind("--data-root=", 0) == 0) {
            overrideValue = argument.substr(std::string("--data-root=").size());
        }
    }

    if (overrideValue.empty()) {
        return "";
    }

    std::filesystem::path dataRoot(overrideValue);
    if (dataRoot.is_relative()) {
        dataRoot = std::filesystem::absolute(dataRoot);
    }

    return dataRoot.lexically_normal().string();
}

bool runStartupChecks() {
    Vector2F vec1(3.0f, 4.0f);
    Vector2F vec2(1.0f, 2.0f);
    const float distance = Vector2F::distanceSquared(vec1, vec2);
    std::cout << "Square distance between vec1 and vec2: " << distance << std::endl;

    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.8f};

    b2WorldId worldId = b2CreateWorld(&worldDef);
    if (!b2World_IsValid(worldId)) {
        std::cerr << "Failed to initialize Box2D world." << std::endl;
        return false;
    }

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = b2_dynamicBody;
    bodyDef.position = {0.0f, 0.0f};

    b2BodyId bodyId = b2CreateBody(worldId, &bodyDef);

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.3f;

    b2Polygon box = b2MakeBox(0.5f, 0.5f);
    b2CreatePolygonShape(bodyId, &shapeDef, &box);

    b2World_Step(worldId, 1.0f / 60.0f, 4);

    b2Vec2 bodyPosition = b2Body_GetPosition(bodyId);
    std::cout << "Box2D initialized. Body position after one step: ("
              << bodyPosition.x << ", " << bodyPosition.y << ")" << std::endl;

    b2DestroyWorld(worldId);
    return true;
}

std::string getRuntimeDataRoot(int argc, char* argv[]) {
    const std::string dataRootOverride = getDataRootOverride(argc, argv);
    if (!dataRootOverride.empty()) {
        return dataRootOverride;
    }

#ifdef IENGINE_ANDROID
    const char* internalStoragePath = SDL_GetAndroidInternalStoragePath();
    if (internalStoragePath != nullptr && internalStoragePath[0] != '\0') {
        return internalStoragePath;
    }
#endif

#ifndef __EMSCRIPTEN__
    if (argc > 0 && argv != nullptr && argv[0] != nullptr && argv[0][0] != '\0') {
        std::filesystem::path executablePath(argv[0]);
        if (executablePath.is_relative()) {
            executablePath = std::filesystem::absolute(executablePath);
        }

        const std::filesystem::path executableDirectory = executablePath.parent_path();
        if (!executableDirectory.empty()
            && (std::filesystem::exists(executableDirectory / "config.xml")
                || std::filesystem::exists(executableDirectory / "Assets"))) {
            return executableDirectory.string();
        }
    }
#endif

    return "";
}

bool shouldEnableTouchControls() {
#if defined(IENGINE_IOS) || defined(IENGINE_ANDROID)
    return true;
#elif defined(__EMSCRIPTEN__)
    return iengineIsTouchDevice();
#else
    return false;
#endif
}

std::string getStartupLevelPath(ProjectManager& projectManager, std::string& errorMessage) {
#ifdef IENGINE_WITH_EDITOR
    errorMessage.clear();
    return projectManager.getStartupLevelPath().string();
#else
    LevelManager& levelManager = LevelManager::getInstance();
    levelManager.load();

    const std::string& initialLevelPath = levelManager.getInitialLevelPath();
    std::filesystem::path resolvedLevelPath;
    if (levelManager.resolveLevelPath(initialLevelPath, resolvedLevelPath, errorMessage)) {
        return resolvedLevelPath.string();
    }

    return {};
#endif
}

std::string getRuntimePath(const std::string& root, const std::string& relativePath) {
    if (root.empty()) {
        return relativePath;
    }

    return (std::filesystem::path(root) / relativePath).string();
}

bool loadWindowConfig(
    const std::string& configPath,
    int& windowWidth,
    int& windowHeight,
    int& framerateLimit,
    std::string& windowTitle
) {
    tinyxml2::XMLDocument config;
    tinyxml2::XMLError loadResult = config.LoadFile(configPath.c_str());
    if (loadResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load " << configPath << ": " << config.ErrorStr() << std::endl;
        return false;
    }

    const tinyxml2::XMLElement* rootConfig = config.FirstChildElement("config");
    if (rootConfig == nullptr) {
        std::cerr << "config.xml is missing config root." << std::endl;
        return false;
    }

    const tinyxml2::XMLElement* windowConfig = rootConfig->FirstChildElement("window");
    if (windowConfig == nullptr) {
        std::cerr << "config.xml is missing config/window." << std::endl;
        return false;
    }

    windowConfig->QueryIntAttribute("width", &windowWidth);
    windowConfig->QueryIntAttribute("height", &windowHeight);
    windowConfig->QueryIntAttribute("framerate", &framerateLimit);
    if (const char* configuredTitle = windowConfig->Attribute("title")) {
        windowTitle = configuredTitle;
    }

    std::cout << "Loaded config.xml. Window: " << windowWidth << "x" << windowHeight
              << ", title: " << windowTitle << std::endl;
    return true;
}

void processPendingScriptLevelLoad() {
    std::string levelName;
    if (!ScriptSystem::getInstance().consumePendingLevelLoad(levelName)) {
        return;
    }

    std::filesystem::path levelPath;
    std::string errorMessage;
    if (!LevelManager::getInstance().resolveLevelPath(levelName, levelPath, errorMessage)) {
        std::cerr << "Failed to resolve Lua level load request for \"" << levelName << "\": "
                  << (errorMessage.empty() ? "unknown error" : errorMessage)
                  << std::endl;
        return;
    }

    Level loadedLevel = Level::createEmpty();
    if (!Level::loadFromFile(levelPath.string(), loadedLevel, errorMessage)) {
        std::cerr << "Failed to load Lua requested level \"" << levelName << "\": "
                  << (errorMessage.empty() ? "unknown error" : errorMessage)
                  << std::endl;
        return;
    }

    Level::loadLevel(std::move(loadedLevel), true);
    Renderer::getInstance().clearTileLayerBatches();
    std::cout << "Loaded Lua requested level: " << levelPath.string() << std::endl;
}
}

Application::Application() = default;

Application::~Application() {
    shutdown();
}

bool Application::initialize(int argc, char* argv[]) {
    if (!runStartupChecks()) {
        return false;
    }

    int windowWidth = 1280;
    int windowHeight = 720;
    int framerateLimit = 60;
    std::string windowTitle = "iEngine(Alpha)";
    const std::string runtimeDataRoot = getRuntimeDataRoot(argc, argv);
    ProjectManager& projectManager = ProjectManager::getInstance();
    projectManager.setDefaultProjectRoot(runtimeDataRoot);

    const std::string configPath = getRuntimePath(runtimeDataRoot, "config.xml");

    if (!loadWindowConfig(configPath, windowWidth, windowHeight, framerateLimit, windowTitle)) {
        return false;
    }

    const std::string backendName = getBackendName(argc, argv);
    std::cout << "Using backend: " << backendName << std::endl;

    if (backendName == "sdl") {
        auto sdlWindowBackend = std::make_unique<SdlWindowBackend>();
        if (!sdlWindowBackend->initialize(windowWidth, windowHeight, windowTitle, framerateLimit)) {
            return false;
        }

        TextureAsset::setTextureRenderBackend(TextureRenderBackend::Sdl, sdlWindowBackend->getRenderer());
        renderBackend = std::make_unique<SdlRenderBackend>(*sdlWindowBackend);
        guiBackend = std::make_unique<SdlGuiBackend>();
        windowBackend = std::move(sdlWindowBackend);
#ifndef IENGINE_SDL_ONLY
    } else if (backendName == "sfml") {
        auto sfmlWindowBackend = std::make_unique<SfmlWindowBackend>();
        if (!sfmlWindowBackend->initialize(windowWidth, windowHeight, windowTitle, framerateLimit)) {
            return false;
        }

        TextureAsset::setTextureRenderBackend(TextureRenderBackend::Sfml, nullptr);
        renderBackend = std::make_unique<SfmlRenderBackend>(*sfmlWindowBackend);
        guiBackend = std::make_unique<SfmlGuiBackend>();
        windowBackend = std::move(sfmlWindowBackend);
#endif
    } else {
#ifdef IENGINE_SDL_ONLY
        std::cerr << "Unknown backend: " << backendName
                  << ". This build supports --backend=sdl." << std::endl;
#else
        std::cerr << "Unknown backend: " << backendName
                  << ". Use --backend=sfml or --backend=sdl." << std::endl;
#endif
        return false;
    }

    if (!guiBackend->initialize(*windowBackend)) {
        shutdown();
        return false;
    }

    AssetManager& assetManager = AssetManager::getInstance();
    if (!assetManager.loadAssets(projectManager.getAssetsPath().string())) {
        std::cerr << "One or more assets failed to load." << std::endl;
    }

    VirtualInputSystem& virtualInputSystem = VirtualInputSystem::getInstance();
    const std::string inputBindingsPath =
        (projectManager.getAssetsPath() / "Input" / "default.input.xml").string();
    std::string inputBindingsError;
    if (!virtualInputSystem.loadBindingsFromFile(inputBindingsPath, inputBindingsError)) {
        std::cerr << inputBindingsError << " Falling back to default input bindings." << std::endl;
        virtualInputSystem.bindDefaultKeyboardMouse();
    }

    TouchControlSystem& touchControlSystem = TouchControlSystem::getInstance();
    touchControlSystem.setEnabled(shouldEnableTouchControls());
    std::cout << "Touch controls: "
              << (touchControlSystem.isEnabled() ? "enabled" : "disabled")
              << std::endl;

    Renderer& renderer = Renderer::getInstance();
    renderer.setRenderBackend(renderBackend.get());
    renderer.setWindowBackend(windowBackend.get());
    renderer.setRenderScale(2.0f);

#ifdef IENGINE_WITH_EDITOR
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Edit);
#else
    EngineState::getInstance().setRuntimeMode(EngineState::RuntimeMode::Play);
#endif

    std::string errorMessage;
    Level startupLevel = Level::createEmpty();
    const std::string startupLevelPath = getStartupLevelPath(projectManager, errorMessage);
    if (!startupLevelPath.empty() && Level::loadFromFile(startupLevelPath, startupLevel, errorMessage)) {
        Level::loadLevel(std::move(startupLevel));
    } else {
        std::cerr << "Failed to load startup level " << startupLevelPath
                  << ": " << (errorMessage.empty() ? "unknown error" : errorMessage)
                  << std::endl;
        Level::loadLevel(Level::createEmpty());
    }

    previousTime = std::chrono::steady_clock::now();
    initialized = true;
    return true;
}

void Application::tick() {
    if (!isRunning()) {
        return;
    }

    const auto currentTime = std::chrono::steady_clock::now();
    const float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
    previousTime = currentTime;

    InputSystem& inputSystem = InputSystem::getInstance();
    RawInputSystem& rawInputSystem = RawInputSystem::getInstance();

    rawInputSystem.beginFrame();
    windowBackend->pollEvents(inputSystem, guiBackend.get());

    TouchControlSystem& touchControlSystem = TouchControlSystem::getInstance();
    const RenderRect viewport = windowBackend->getViewport();
    touchControlSystem.updateFromRawInput(rawInputSystem, viewport.width, viewport.height);

    const bool uiConsumedInput = UISystem::getInstance().processInput(
        rawInputSystem,
        windowBackend.get()
    );

    VirtualInputSystem& virtualInputSystem = VirtualInputSystem::getInstance();
    virtualInputSystem.updateFromRawInput(rawInputSystem, uiConsumedInput);
    virtualInputSystem.updateFromTouchControls(touchControlSystem, uiConsumedInput);

    if (!EngineState::getInstance().isGamePaused()) {
        PhysicsSystem::getInstance().update(deltaTime);
        Level::getCurrentLevel().update(deltaTime);
        Renderer::getInstance().update(deltaTime);
    }

    processPendingScriptLevelLoad();

    guiBackend->update(deltaTime);
    Renderer::getInstance().updateEditorOnly(deltaTime);

    windowBackend->beginFrame(RenderConstants::ClearColor);
    Renderer::getInstance().render();
    guiBackend->render(inputSystem);
    windowBackend->endFrame();

    rawInputSystem.endFrame();
}

void Application::shutdown() {
    if (!initialized && windowBackend == nullptr && guiBackend == nullptr && renderBackend == nullptr) {
        return;
    }

    Level::getCurrentLevel().clearEntities();
    Renderer::getInstance().clearTileLayerBatches();
    AssetManager::getInstance().clearAssets();

    if (guiBackend != nullptr) {
        guiBackend->shutdown();
    }

    if (windowBackend != nullptr) {
        windowBackend->shutdown();
    }

    Renderer::getInstance().setRenderBackend(nullptr);
    Renderer::getInstance().setWindowBackend(nullptr);
#ifdef IENGINE_SDL_ONLY
    TextureAsset::setTextureRenderBackend(TextureRenderBackend::Sdl, nullptr);
#else
    TextureAsset::setTextureRenderBackend(TextureRenderBackend::Sfml, nullptr);
#endif

    guiBackend.reset();
    renderBackend.reset();
    windowBackend.reset();
    initialized = false;
}

bool Application::isRunning() const {
    return initialized && windowBackend != nullptr && windowBackend->isOpen();
}
