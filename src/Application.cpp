#include "Application.h"

#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/IGuiBackend.h"
#include "system/IRenderBackend.h"
#include "system/IWindowBackend.h"
#include "system/InputSystem.h"
#include "system/PhysicsSystem.h"
#include "system/RawInputSystem.h"
#include "system/Renderer.h"
#include "system/sdl/SdlGuiBackend.h"
#include "system/sdl/SdlRenderBackend.h"
#include "system/sdl/SdlWindowBackend.h"
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

#include <filesystem>
#include <iostream>
#include <memory>
#include <string>
#include <utility>

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

std::string getRuntimeDataRoot() {
#ifdef IENGINE_ANDROID
    const char* internalStoragePath = SDL_GetAndroidInternalStoragePath();
    if (internalStoragePath != nullptr && internalStoragePath[0] != '\0') {
        return internalStoragePath;
    }
#endif

    return "";
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
    const std::string runtimeDataRoot = getRuntimeDataRoot();
    const std::string configPath = getRuntimePath(runtimeDataRoot, "config.xml");
    const std::string assetsPath = getRuntimePath(runtimeDataRoot, "Assets");

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
    if (!assetManager.loadAssets(assetsPath)) {
        std::cerr << "One or more assets failed to load." << std::endl;
    }

    VirtualInputSystem& virtualInputSystem = VirtualInputSystem::getInstance();
    const std::string inputBindingsPath = getRuntimePath(runtimeDataRoot, "Assets/Input/default.input.xml");
    std::string inputBindingsError;
    if (!virtualInputSystem.loadBindingsFromFile(inputBindingsPath, inputBindingsError)) {
        std::cerr << inputBindingsError << " Falling back to default input bindings." << std::endl;
        virtualInputSystem.bindDefaultKeyboardMouse();
    }

    Renderer& renderer = Renderer::getInstance();
    renderer.setRenderBackend(renderBackend.get());
    renderer.setWindowBackend(windowBackend.get());
    renderer.setRenderScale(2.0f);

    std::string errorMessage;
    Level startupLevel = Level::createEmpty();
    if (Level::loadFromFile(Level::getCurrentLevelPath(), startupLevel, errorMessage)) {
        Level::loadLevel(std::move(startupLevel));
    } else {
        std::cerr << "Failed to load startup level " << Level::getCurrentLevelPath()
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
    VirtualInputSystem::getInstance().updateFromRawInput(rawInputSystem);

    PhysicsSystem::getInstance().update(deltaTime);
    Level::getCurrentLevel().update(deltaTime);
    Renderer::getInstance().update(deltaTime);

    guiBackend->update(deltaTime);

    windowBackend->beginFrame(RenderColor{45, 45, 50, 255});
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
