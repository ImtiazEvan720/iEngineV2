#include "imgui.h"
#include "imgui-SFML.h"
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
#include "game/Brick.h"
#include "misc/TextureAsset.h"
#include <box2d/box2d.h>
#include <SFML/Graphics.hpp>
#include <SFML/Window/Event.hpp>
#include <iostream>
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
    const char* windowTitle = "iEngine(Alpha)";

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

    // 1. Create your SFML Window.
    // SFML Graphics uses fixed-function OpenGL internally, so avoid a core profile here.
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 2;
    settings.minorVersion = 1;

    sf::RenderWindow window(
        sf::VideoMode({static_cast<unsigned int>(windowWidth), static_cast<unsigned int>(windowHeight)}),
        windowTitle,
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );

    window.setFramerateLimit(static_cast<unsigned int>(framerateLimit));
    
    if (!ImGui::SFML::Init(window)) {
        return 1;
    }

    AssetManager& assetManager = AssetManager::getInstance();
    if (!assetManager.loadAssets()) {
        std::cerr << "One or more assets failed to load." << std::endl;
    }

    TextureAsset* spriteSheetAsset = assetManager.getTextureAssetByName(
        "NES - Battle City (JPN) - Miscellaneous - General Sprites.png"
    );
    if (spriteSheetAsset == nullptr || spriteSheetAsset->getTexture() == nullptr) {
        std::cerr << "Failed to find Battle City sprite sheet texture." << std::endl;
        ImGui::SFML::Shutdown();
        return 1;
    }

    LevelAsset* levelAsset = assetManager.getLevelAssetByName("custom.tmx");
    if (levelAsset == nullptr) {
        std::cerr << "Failed to find Custom.tmx level asset." << std::endl;
        ImGui::SFML::Shutdown();
        return 1;
    }

    Renderer& renderer = Renderer::getInstance();
    renderer.setWindow(&window);
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

    Sprite testSprite(spriteSheetAsset->getTexture(), sf::FloatRect({width*0, height*0}, {width, height}));
    testSprite.setSize(Vector2F(scaledWidth, scaledHeight));

    Sprite testSprite2(spriteSheetAsset->getTexture(), sf::FloatRect({width*1, height*0}, {width, height}));
    testSprite2.setSize(Vector2F(scaledWidth, scaledHeight));


    Animation anim = Animation(0.1f);
    anim.addFrame(testSprite);
    anim.addFrame(testSprite2);

    AnimationComponent& animationComponent = testEntity.addComponent<AnimationComponent>(anim);
    animationComponent.pause();
    testEntity.addComponent<PlayerController>();

    
    #pragma region Add Brick Entity

    Entity& brick = level.createEntity();
    Sprite brickSprite(spriteSheetAsset->getTexture(), sf::FloatRect({width*16, height*0}, {width, height}));
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

    sf::CircleShape greenCircle(100.0f);
    greenCircle.setFillColor(sf::Color::Green);
    greenCircle.setPosition({200.0f, 160.0f});

    ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    sf::Clock deltaClock;

    while (window.isOpen()) {

        sf::Time dt = deltaClock.restart();
        float deltaTime = dt.asSeconds();

        while (const std::optional event = window.pollEvent()) {
            ImGui::SFML::ProcessEvent(window, *event);
            inputSystem.processEvent(*event);

            if (event->is<sf::Event::Closed>()) {
                window.close();
            }
        }

        PhysicsSystem::getInstance().update(deltaTime);
        level.update(deltaTime);
        renderer.update(deltaTime);
        ImGui::SFML::Update(window, dt);

        ImGui::Begin("Custom Integration Window");
        ImGui::Text("Hello, World! I am running through ImGui-SFML.");
        ImGui::Text("Input: %s", inputSystem.getLastInputText().c_str());
        if (ImGui::Button("Test Click")) {
            // Logic
        }
        ImGui::End();

        window.clear(sf::Color(45, 45, 50)); // Clear SFML Canvas
        window.draw(greenCircle);
        renderer.render();
        ImGui::SFML::Render(window);
        window.display();
    }

    level.clearEntities();
    renderer.clearTileLayerBatches();
    assetManager.clearAssets();
    ImGui::SFML::Shutdown();

    return 0;
}
