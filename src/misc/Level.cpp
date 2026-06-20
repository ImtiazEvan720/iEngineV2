#include "misc/Level.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/InputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <iostream>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

namespace {
std::string currentLevelPath = "Assets/Levels/current.ilevel";

bool parseBool(const char* value, bool fallback) {
    if (value == nullptr) {
        return fallback;
    }

    const std::string text = value;
    return text == "true" || text == "1" || text == "True" || text == "TRUE";
}

CollisionComponent::BodyType parseBodyType(const std::string& value) {
    if (value == "Dynamic" || value == "dynamic") {
        return CollisionComponent::BodyType::Dynamic;
    }

    if (value == "Kinematic" || value == "kinematic") {
        return CollisionComponent::BodyType::Kinematic;
    }

    return CollisionComponent::BodyType::Static;
}

const char* boolText(bool value) {
    return value ? "true" : "false";
}

const char* bodyTypeToString(CollisionComponent::BodyType bodyType) {
    switch (bodyType) {
        case CollisionComponent::BodyType::Kinematic:
            return "Kinematic";
        case CollisionComponent::BodyType::Dynamic:
            return "Dynamic";
        case CollisionComponent::BodyType::Static:
        default:
            return "Static";
    }
}

std::string getTextureAssetName(RenderTextureHandle textureHandle) {
    if (textureHandle == nullptr) {
        return "";
    }

    for (const auto& asset : AssetManager::getInstance().getAssets()) {
        const auto* textureAsset = dynamic_cast<const TextureAsset*>(asset.get());
        if (textureAsset != nullptr && textureAsset->getTextureHandle() == textureHandle) {
            return std::filesystem::path(textureAsset->getPath()).filename().string();
        }
    }

    return "";
}

tinyxml2::XMLElement* addComponentElement(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const char* type
) {
    tinyxml2::XMLElement* component = document.NewElement("component");
    component->SetAttribute("type", type);
    entityElement.InsertEndChild(component);
    return component;
}

void setSpriteAttributes(tinyxml2::XMLElement& element, const Sprite& sprite) {
    const RenderRect& source = sprite.getSourceRect();
    const Vector2F& size = sprite.getSize();
    const Vector2F& origin = sprite.getOrigin();

    element.SetAttribute("texture", getTextureAssetName(sprite.getTextureHandle()).c_str());
    element.SetAttribute("sourceX", source.x);
    element.SetAttribute("sourceY", source.y);
    element.SetAttribute("sourceWidth", source.width);
    element.SetAttribute("sourceHeight", source.height);
    element.SetAttribute("sizeX", size.x);
    element.SetAttribute("sizeY", size.y);
    element.SetAttribute("originX", origin.x);
    element.SetAttribute("originY", origin.y);
}

void saveTransformComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const TransformComponent& transform
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "TransformComponent");
    const Vector2F& position = transform.getPosition();
    component->SetAttribute("x", position.x);
    component->SetAttribute("y", position.y);
    component->SetAttribute("rotation", transform.getRotation());
}

void saveSpriteComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const SpriteComponent& spriteComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "SpriteComponent");
    setSpriteAttributes(*component, spriteComponent.getSprite());
}

void saveAnimationComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const AnimationComponent& animationComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "AnimationComponent");
    const Animation& animation = animationComponent.getAnimation();
    component->SetAttribute("frameDuration", animation.getFrameDuration());
    component->SetAttribute("playing", boolText(animationComponent.isPlaying()));

    for (std::size_t frameIndex = 0; frameIndex < animation.getFrameCount(); ++frameIndex) {
        tinyxml2::XMLElement* frame = document.NewElement("frame");
        setSpriteAttributes(*frame, animation.getFrame(frameIndex));
        component->InsertEndChild(frame);
    }
}

void saveCollisionComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const CollisionComponent& collisionComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "CollisionComponent");
    const Vector2F& offset = collisionComponent.getOffset();
    component->SetAttribute("name", collisionComponent.getName().c_str());
    component->SetAttribute("width", collisionComponent.getWidth());
    component->SetAttribute("height", collisionComponent.getHeight());
    component->SetAttribute("offsetX", offset.x);
    component->SetAttribute("offsetY", offset.y);
    component->SetAttribute("bodyType", bodyTypeToString(collisionComponent.getBodyType()));
    component->SetAttribute("isSensor", boolText(collisionComponent.isSensor()));
}

void saveScriptComponent(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const ScriptComponent& scriptComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "ScriptComponent");
    component->SetAttribute("path", scriptComponent.getScriptPath().c_str());
}

void addMarkerComponent(tinyxml2::XMLDocument& document, tinyxml2::XMLElement& entityElement, const char* type) {
    addComponentElement(document, entityElement, type);
}

TextureAsset* getTextureAsset(const tinyxml2::XMLElement& element) {
    const char* textureName = element.Attribute("texture");
    if (textureName == nullptr || textureName[0] == '\0') {
        return nullptr;
    }

    return AssetManager::getInstance().getTextureAssetByName(textureName);
}

Sprite makeSpriteFromAttributes(const tinyxml2::XMLElement& element, TextureAsset& textureAsset) {
    RenderRect source;
    source.x = element.FloatAttribute("sourceX", 0.0f);
    source.y = element.FloatAttribute("sourceY", 0.0f);
    source.width = element.FloatAttribute("sourceWidth", 0.0f);
    source.height = element.FloatAttribute("sourceHeight", 0.0f);

    Sprite sprite(textureAsset.getTextureHandle(), source);
    sprite.setSize(Vector2F(
        element.FloatAttribute("sizeX", source.width),
        element.FloatAttribute("sizeY", source.height)
    ));
    sprite.setOrigin(Vector2F(
        element.FloatAttribute("originX", source.width * 0.5f),
        element.FloatAttribute("originY", source.height * 0.5f)
    ));

    return sprite;
}

bool addSpriteComponentFromElement(const tinyxml2::XMLElement& component, Entity& entity, std::string& errorMessage) {
    TextureAsset* textureAsset = getTextureAsset(component);
    if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
        errorMessage = "Level sprite texture is missing.";
        return false;
    }

    entity.addComponent<SpriteComponent>(makeSpriteFromAttributes(component, *textureAsset));
    return true;
}

bool addAnimationComponentFromElement(const tinyxml2::XMLElement& component, Entity& entity, std::string& errorMessage) {
    Animation animation(component.FloatAttribute("frameDuration", 0.1f));

    for (const tinyxml2::XMLElement* frame = component.FirstChildElement("frame");
         frame != nullptr;
         frame = frame->NextSiblingElement("frame")) {
        TextureAsset* textureAsset = getTextureAsset(*frame);
        if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
            errorMessage = "Level animation frame texture is missing.";
            return false;
        }

        animation.addFrame(makeSpriteFromAttributes(*frame, *textureAsset));
    }

    if (!animation.hasFrames()) {
        errorMessage = "Level animation component has no frames.";
        return false;
    }

    AnimationComponent& animationComponent = entity.addComponent<AnimationComponent>(animation);
    if (!parseBool(component.Attribute("playing"), true)) {
        animationComponent.pause();
    }

    return true;
}

bool addComponentFromElement(const tinyxml2::XMLElement& component, Entity& entity, std::string& errorMessage) {
    const char* type = component.Attribute("type");
    if (type == nullptr) {
        return true;
    }

    const std::string componentType(type);
    if (componentType == "TransformComponent") {
        return true;
    }

    if (componentType == "SpriteComponent") {
        return addSpriteComponentFromElement(component, entity, errorMessage);
    }

    if (componentType == "AnimationComponent") {
        return addAnimationComponentFromElement(component, entity, errorMessage);
    }

    if (componentType == "CollisionComponent") {
        CollisionComponent& collisionComponent = entity.addComponent<CollisionComponent>(
            component.FloatAttribute("width", 1.0f),
            component.FloatAttribute("height", 1.0f),
            parseBodyType(component.Attribute("bodyType") == nullptr ? "Static" : component.Attribute("bodyType")),
            parseBool(component.Attribute("isSensor"), false),
            component.Attribute("name") == nullptr ? "Collider" : component.Attribute("name")
        );
        collisionComponent.setOffset(Vector2F(
            component.FloatAttribute("offsetX", 0.0f),
            component.FloatAttribute("offsetY", 0.0f)
        ));
        return true;
    }

    if (componentType == "ScriptComponent") {
        const char* path = component.Attribute("path");
        if (path != nullptr && path[0] != '\0') {
            entity.addComponent<ScriptComponent>(path);
        }
        return true;
    }

    if (componentType == "PlayerController") {
        entity.addComponent<ScriptComponent>("Assets/Scripts/player_controller.lua");
        return true;
    }

    if (componentType == "Brick") {
        entity.addComponent<Brick>();
        return true;
    }

    if (componentType == "Bullet") {
        entity.addComponent<Bullet>();
        return true;
    }

    std::cerr << "Unknown level component: " << componentType << std::endl;
    return true;
}
}

Level& Level::getCurrentLevel() {
    static Level currentLevel;
    return currentLevel;
}

Level Level::createEmpty() {
    return Level();
}

Level& Level::createNewLevel() {
    Level level = createEmpty();
    level.sourcePath = "Assets/Levels/current.ilevel";
    loadLevel(std::move(level));
    currentLevelPath = "Assets/Levels/current.ilevel";
    return getCurrentLevel();
}

bool Level::saveCurrentLevel(std::string& errorMessage) {
    return saveCurrentLevel(currentLevelPath, errorMessage);
}

bool Level::saveCurrentLevel(const std::string& path, std::string& errorMessage) {
    namespace fs = std::filesystem;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* levelElement = document.NewElement("level");
    levelElement->SetAttribute("version", 1);
    document.InsertEndChild(levelElement);

    tinyxml2::XMLElement* entitiesElement = document.NewElement("entities");
    levelElement->InsertEndChild(entitiesElement);

    int savedEntityCount = 0;
    const Level& level = getCurrentLevel();
    for (const Entity& entity : level.getEntities()) {
        if (entity.isDestroyed()) {
            continue;
        }

        tinyxml2::XMLElement* entityElement = document.NewElement("entity");
        entityElement->SetAttribute("id", entity.getId());
        entityElement->SetAttribute("name", entity.getName().c_str());
        entityElement->SetAttribute("tag", entity.getTag().c_str());
        entityElement->SetAttribute("enabled", entity.isEnabled());
        if (const Entity* parent = entity.getParent()) {
            entityElement->SetAttribute("parentId", parent->getId());
        }
        entitiesElement->InsertEndChild(entityElement);

        if (const TransformComponent* transform = entity.getComponent<TransformComponent>()) {
            saveTransformComponent(document, *entityElement, *transform);
        }

        if (const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
            saveSpriteComponent(document, *entityElement, *spriteComponent);
        }

        if (const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>()) {
            saveAnimationComponent(document, *entityElement, *animationComponent);
        }

        if (const CollisionComponent* collisionComponent = entity.getComponent<CollisionComponent>()) {
            saveCollisionComponent(document, *entityElement, *collisionComponent);
        }

        if (const ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>()) {
            saveScriptComponent(document, *entityElement, *scriptComponent);
        }

        if (entity.getComponent<PlayerController>() != nullptr) {
            addMarkerComponent(document, *entityElement, "PlayerController");
        }

        if (entity.getComponent<Brick>() != nullptr) {
            addMarkerComponent(document, *entityElement, "Brick");
        }

        if (entity.getComponent<Bullet>() != nullptr) {
            addMarkerComponent(document, *entityElement, "Bullet");
        }

        ++savedEntityCount;
    }

    entitiesElement->SetAttribute("count", savedEntityCount);

    const fs::path outputPath(path);
    std::error_code directoryError;
    if (!outputPath.parent_path().empty()) {
        fs::create_directories(outputPath.parent_path(), directoryError);
        if (directoryError) {
            errorMessage = "Failed to create level directory: " + directoryError.message();
            return false;
        }
    }

    const tinyxml2::XMLError result = document.SaveFile(path.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to save level: " + std::string(document.ErrorStr());
        return false;
    }

    currentLevelPath = path;
    getCurrentLevel().sourcePath = path;
    errorMessage.clear();
    return true;
}

bool Level::loadFromFile(const std::string& path, Level& level, std::string& errorMessage) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load level: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* levelElement = document.FirstChildElement("level");
    if (levelElement == nullptr) {
        errorMessage = "Level file is missing level root.";
        return false;
    }

    const tinyxml2::XMLElement* entitiesElement = levelElement->FirstChildElement("entities");

    Level loadedLevel = createEmpty();
    loadedLevel.sourcePath = path;
    std::unordered_map<int, Entity*> entitiesBySavedId;

    struct PendingParentLink {
        Entity* entity = nullptr;
        int parentId = -1;
    };
    std::vector<PendingParentLink> pendingParentLinks;

    for (const tinyxml2::XMLElement* entityElement = entitiesElement == nullptr ? nullptr : entitiesElement->FirstChildElement("entity");
         entityElement != nullptr;
         entityElement = entityElement->NextSiblingElement("entity")) {
        Entity& entity = loadedLevel.createEntity();
        entity.setName(entityElement->Attribute("name") == nullptr ? "Entity" : entityElement->Attribute("name"));
        entity.setTag(entityElement->Attribute("tag") == nullptr ? "Default" : entityElement->Attribute("tag"));
        entity.setEnabled(entityElement->BoolAttribute("enabled", true));

        const int savedEntityId = entityElement->IntAttribute("id", -1);
        if (savedEntityId >= 0) {
            entitiesBySavedId[savedEntityId] = &entity;
        }

        const int parentId = entityElement->IntAttribute("parentId", -1);
        if (parentId >= 0) {
            pendingParentLinks.push_back(PendingParentLink{&entity, parentId});
        }

        const tinyxml2::XMLElement* transformElement = nullptr;
        const tinyxml2::XMLElement* collisionElement = nullptr;
        for (const tinyxml2::XMLElement* component = entityElement->FirstChildElement("component");
             component != nullptr;
             component = component->NextSiblingElement("component")) {
            const char* type = component->Attribute("type");
            if (type == nullptr) {
                continue;
            }

            const std::string componentType(type);
            if (componentType == "TransformComponent") {
                transformElement = component;
            } else if (componentType == "CollisionComponent") {
                collisionElement = component;
            }
        }

        if (transformElement != nullptr) {
            entity.addComponent<TransformComponent>(
                Vector2F(
                    transformElement->FloatAttribute("x", 0.0f),
                    transformElement->FloatAttribute("y", 0.0f)
                ),
                transformElement->FloatAttribute("rotation", 0.0f)
            );
        } else if (collisionElement != nullptr) {
            entity.addComponent<TransformComponent>(Vector2F(0.0f, 0.0f), 0.0f);
        }

        for (const tinyxml2::XMLElement* component = entityElement->FirstChildElement("component");
             component != nullptr;
             component = component->NextSiblingElement("component")) {
            if (!addComponentFromElement(*component, entity, errorMessage)) {
                return false;
            }
        }
    }

    for (const PendingParentLink& pendingLink : pendingParentLinks) {
        const auto parentIterator = entitiesBySavedId.find(pendingLink.parentId);
        if (pendingLink.entity != nullptr && parentIterator != entitiesBySavedId.end()) {
            pendingLink.entity->setParent(parentIterator->second, false);
        }
    }

    level = std::move(loadedLevel);
    errorMessage.clear();
    return true;
}

bool Level::loadCurrentLevel(std::string& errorMessage) {
    Level level = createEmpty();
    if (!loadFromFile(currentLevelPath, level, errorMessage)) {
        return false;
    }

    loadLevel(std::move(level));
    return true;
}

void Level::loadLevel(Level&& level) {
    Level& currentLevel = getCurrentLevel();
    currentLevel = std::move(level);

    if (!currentLevel.sourcePath.empty()) {
        currentLevelPath = currentLevel.sourcePath;
    }

    for (Entity& entity : currentLevel.entities) {
        entity.addInputListeners(InputSystem::getInstance());
    }
}

const std::string& Level::getCurrentLevelPath() {
    return currentLevelPath;
}

void Level::setCurrentLevelPath(const std::string& path) {
    currentLevelPath = path;
    getCurrentLevel().sourcePath = path;
}

Entity& Level::createEntity() {
    entities.emplace_back();
    return entities.back();
}

Entity& Level::addEntity(Entity entity) {
    entities.push_back(std::move(entity));
    return entities.back();
}

bool Level::removeEntity(std::size_t index) {
    if (index >= entities.size()) {
        return false;
    }

    entities[index].destroy();
    entities.erase(entities.begin() + static_cast<std::deque<Entity>::difference_type>(index));
    return true;
}

bool Level::destroyEntity(Entity& entity) {
    return destroyEntity(&entity);
}

bool Level::destroyEntity(Entity* entity) {
    if (entity == nullptr) {
        return false;
    }

    for (Entity& storedEntity : entities) {
        if (&storedEntity == entity) {
            storedEntity.destroy();
            return true;
        }
    }

    return false;
}

bool Level::destroyEntityById(int id) {
    for (Entity& entity : entities) {
        if (entity.getId() == id) {
            entity.destroy();
            return true;
        }
    }

    return false;
}

void Level::cleanupDestroyedEntities() {
    entities.erase(
        std::remove_if(
            entities.begin(),
            entities.end(),
            [](const Entity& entity) {
                return entity.isDestroyed();
            }
        ),
        entities.end()
    );
}

void Level::clearEntities() {
    for (Entity& entity : entities) {
        entity.destroy();
    }

    entities.clear();
}

Entity* Level::getEntity(std::size_t index) {
    if (index >= entities.size()) {
        return nullptr;
    }

    return &entities[index];
}

const Entity* Level::getEntity(std::size_t index) const {
    if (index >= entities.size()) {
        return nullptr;
    }

    return &entities[index];
}

std::deque<Entity>& Level::getEntities() {
    return entities;
}

const std::deque<Entity>& Level::getEntities() const {
    return entities;
}

void Level::update(float deltaTime) {
    for (Entity& entity : entities) {
        if (entity.isDestroyed() || !entity.isEnabled()) {
            continue;
        }

        entity.update(deltaTime);
    }

    cleanupDestroyedEntities();
}
