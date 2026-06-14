#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "components/PlayerController.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/Animation.h"
#include "math/Vector2F.h"
#include "system/AssetManager.h"
#include "system/InputSystem.h"
#include "system/PhysicsSystem.h"
#include "system/Renderer.h"
#include "system/sfml/SfmlGuiBackend.h"
#include "system/sfml/SfmlRenderBackend.h"
#include "system/sfml/SfmlWindowBackend.h"
#include "game/Brick.h"
#include "misc/TextureAsset.h"
#include <box2d/box2d.h>
#include <chrono>
#include <iostream>
#include <string>
#include <tinyxml2.h>

int main() {
    //check Vector2F custom class
    Vector2F vec1(3.0f, 4.0f);
    Vector2F vec2(1.0f, 2.0f);
    float distance = Vector2F::distanceSquared(vec1, vec2);
    std::cout << "Square distance between vec1 and vec2: " << distance << std::endl;

    #pragma region Box2D Initialization and Test
    // Check Box2D initialization and a single physics step.
    b2WorldDef worldDef = b2DefaultWorldDef();
    worldDef.gravity = {0.0f, 9.8f};

    b2WorldId worldId = b2CreateWorld(&worldDef);
    if (!b2World_IsValid(worldId)) {
        std::cerr << "Failed to initialize Box2D world." << std::endl;
        return 1;
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

    #pragma endregion

    int windowWidth = 1280;
    int windowHeight = 720;
    int framerateLimit = 60;
    std::string windowTitle = "iEngine(Alpha)";

    tinyxml2::XMLDocument config;
    tinyxml2::XMLError loadResult = config.LoadFile("config.xml");
    if (loadResult != tinyxml2::XML_SUCCESS) {
        std::cerr << "Failed to load config.xml: " << config.ErrorStr() << std::endl;
        return 1;
    }

    const tinyxml2::XMLElement* rootConfig = config.FirstChildElement("config");
    if (rootConfig == nullptr) {
        std::cerr << "config.xml is missing config root." << std::endl;
        return 1;
    }

    const tinyxml2::XMLElement* windowConfig = rootConfig->FirstChildElement("window");
    if (windowConfig == nullptr) {
        std::cerr << "config.xml is missing config/window." << std::endl;
        return 1;
    }

    windowConfig->QueryIntAttribute("width", &windowWidth);
    windowConfig->QueryIntAttribute("height", &windowHeight);
    windowConfig->QueryIntAttribute("framerate", &framerateLimit);
    if (const char* configuredTitle = windowConfig->Attribute("title")) {
        windowTitle = configuredTitle;
    }

    std::cout << "Loaded config.xml. Window: " << windowWidth << "x" << windowHeight
              << ", title: " << windowTitle << std::endl;

    SfmlWindowBackend windowBackend;
    if (!windowBackend.initialize(windowWidth, windowHeight, windowTitle, framerateLimit)) {
        return 1;
    }

    SfmlGuiBackend guiBackend;
    if (!guiBackend.initialize(windowBackend)) {
        windowBackend.shutdown();
        return 1;
    }

    AssetManager& assetManager = AssetManager::getInstance();
    if (!assetManager.loadAssets()) {
        std::cerr << "One or more assets failed to load." << std::endl;
    }

    TextureAsset* spriteSheetAsset = assetManager.getTextureAssetByName(
        "NES - Battle City (JPN) - Miscellaneous - General Sprites.png"
    );
    if (spriteSheetAsset == nullptr || spriteSheetAsset->getTextureHandle() == nullptr) {
        std::cerr << "Failed to find Battle City sprite sheet texture." << std::endl;
        guiBackend.shutdown();
        windowBackend.shutdown();
        return 1;
    }

    LevelAsset* levelAsset = assetManager.getLevelAssetByName("custom.tmx");
    if (levelAsset == nullptr) {
        std::cerr << "Failed to find Custom.tmx level asset." << std::endl;
        guiBackend.shutdown();
        windowBackend.shutdown();
        return 1;
    }

    SfmlRenderBackend renderBackend(windowBackend);

    Renderer& renderer = Renderer::getInstance();
    renderer.setRenderBackend(&renderBackend);
    renderer.setWindowBackend(&windowBackend);
    renderer.setRenderScale(2.0f);

    Level& level = Level::getCurrentLevel();
    levelAsset->print();
    level.printEntityPreviewFromAsset(*levelAsset);

    Entity& testEntity = level.createEntity();
    testEntity.setName("Player1");
    testEntity.setTag("Player");
    testEntity.addComponent<TransformComponent>(Vector2F(420.0f, 260.0f), 0.0f);

    const float height = 16.0f;
    const float width = 16.0f;
    const float scaledWidth = width * renderer.getRenderScale();
    const float scaledHeight = height * renderer.getRenderScale();

    Sprite testSprite(
        spriteSheetAsset->getTextureHandle(),
        RenderRect{width * 0.0f, height * 0.0f, width, height}
    );
    testSprite.setSize(Vector2F(scaledWidth, scaledHeight));

    Sprite testSprite2(
        spriteSheetAsset->getTextureHandle(),
        RenderRect{width * 1.0f, height * 0.0f, width, height}
    );
    testSprite2.setSize(Vector2F(scaledWidth, scaledHeight));


    Animation anim = Animation(0.1f);
    anim.addFrame(testSprite);
    anim.addFrame(testSprite2);

    AnimationComponent& animationComponent = testEntity.addComponent<AnimationComponent>(anim);
    animationComponent.pause();
    testEntity.addComponent<PlayerController>();

    
    #pragma region Add Brick Entity

    Entity& brick = level.createEntity();
    Sprite brickSprite(
        spriteSheetAsset->getTextureHandle(),
        RenderRect{width * 16.0f, height * 0.0f, width, height}
    );
    brickSprite.setSize(Vector2F(scaledWidth, scaledHeight));
    brick.addComponent<SpriteComponent>(brickSprite);
    brick.addComponent<TransformComponent>(Vector2F(520.0f, 260.0f), 0.0f);
    brick.addComponent<CollisionComponent>(
        scaledWidth,
        scaledHeight,
        CollisionComponent::BodyType::Static,
        false,
        "Brick"
    );
    brick.addComponent<Brick>();

    #pragma endregion

    InputSystem& inputSystem = InputSystem::getInstance();
    for (Entity& entity : level.getEntities()) {
        entity.addInputListeners(inputSystem);
    }

    renderer.buildTileLayerBatches(*levelAsset);

    auto previousTime = std::chrono::steady_clock::now();

    while (windowBackend.isOpen()) {
        const auto currentTime = std::chrono::steady_clock::now();
        const float deltaTime = std::chrono::duration<float>(currentTime - previousTime).count();
        previousTime = currentTime;

        windowBackend.pollEvents(inputSystem, &guiBackend);

        PhysicsSystem::getInstance().update(deltaTime);
        level.update(deltaTime);
        renderer.update(deltaTime);

        guiBackend.update(deltaTime);

        windowBackend.beginFrame(RenderColor{45, 45, 50, 255});
        renderer.render();
        guiBackend.render(inputSystem);
        windowBackend.endFrame();
    }

    level.clearEntities();
    renderer.clearTileLayerBatches();
    assetManager.clearAssets();
    guiBackend.shutdown();
    windowBackend.shutdown();

    return 0;
}
