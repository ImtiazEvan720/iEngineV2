#include "editor/PrefabSerializer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "game/Brick.h"
#include "game/Bullet.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "misc/TextureAsset.h"
#include "system/AssetManager.h"
#include "system/InputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <sstream>

namespace {
const char* boolText(bool value) {
    return value ? "true" : "false";
}

bool parseBool(const char* value, bool fallback = false) {
    if (value == nullptr) {
        return fallback;
    }

    std::string text(value);
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });

    return text == "true" || text == "1" || text == "yes";
}


bool loadPrefabPreview(const std::string& prefabPath, ImTextureID& textureId, ImVec2& uv0, ImVec2& uv1) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(prefabPath.c_str()) != tinyxml2::XML_SUCCESS) {
        return false;
    }

    const tinyxml2::XMLElement* prefab = document.FirstChildElement("prefab");
    const tinyxml2::XMLElement* entity = prefab ? prefab->FirstChildElement("entity") : nullptr;
    if (entity == nullptr) {
        return false;
    }

    const tinyxml2::XMLElement* spriteComponent = nullptr;

    for (const tinyxml2::XMLElement* component = entity->FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type != nullptr && std::string(type) == "SpriteComponent") {
            spriteComponent = component;
            break;
        }
    }

    if (spriteComponent == nullptr) {
        return false;
    }

    const char* textureName = spriteComponent->Attribute("texture");
    if (textureName == nullptr) {
        return false;
    }

    TextureAsset* textureAsset =
        AssetManager::getInstance().getTextureAssetByName(textureName);

    if (textureAsset == nullptr || textureAsset->getImGuiTextureId() == ImTextureID{}) {
        return false;
    }

    const float sourceX = spriteComponent->FloatAttribute("sourceX");
    const float sourceY = spriteComponent->FloatAttribute("sourceY");
    const float sourceW = spriteComponent->FloatAttribute("sourceWidth");
    const float sourceH = spriteComponent->FloatAttribute("sourceHeight");

    textureId = textureAsset->getImGuiTextureId();

    uv0 = ImVec2(
        sourceX / static_cast<float>(textureAsset->getWidth()),
        sourceY / static_cast<float>(textureAsset->getHeight())
    );

    uv1 = ImVec2(
        (sourceX + sourceW) / static_cast<float>(textureAsset->getWidth()),
        (sourceY + sourceH) / static_cast<float>(textureAsset->getHeight())
    );

    return true;
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

CollisionComponent::BodyType bodyTypeFromString(const char* value) {
    if (value == nullptr) {
        return CollisionComponent::BodyType::Static;
    }

    std::string text(value);
    std::transform(text.begin(), text.end(), text.begin(),
                   [](unsigned char character) {
                       return static_cast<char>(std::tolower(character));
                   });

    if (text == "kinematic") {
        return CollisionComponent::BodyType::Kinematic;
    }

    if (text == "dynamic") {
        return CollisionComponent::BodyType::Dynamic;
    }

    return CollisionComponent::BodyType::Static;
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

TextureAsset* getTextureAsset(const tinyxml2::XMLElement& element) {
    const char* textureName = element.Attribute("texture");
    if (textureName == nullptr) {
        return nullptr;
    }

    return AssetManager::getInstance().getTextureAssetByName(textureName);
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

void saveTransform(
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

void saveSprite(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const SpriteComponent& spriteComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "SpriteComponent");
    setSpriteAttributes(*component, spriteComponent.getSprite());
}

void saveAnimation(
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

void saveCollision(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& entityElement,
    const CollisionComponent& collisionComponent
) {
    tinyxml2::XMLElement* component = addComponentElement(document, entityElement, "CollisionComponent");
    component->SetAttribute("name", collisionComponent.getName().c_str());
    component->SetAttribute("width", collisionComponent.getWidth());
    component->SetAttribute("height", collisionComponent.getHeight());
    component->SetAttribute("bodyType", bodyTypeToString(collisionComponent.getBodyType()));
    component->SetAttribute("isSensor", boolText(collisionComponent.isSensor()));
}

void saveScript(
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

bool loadSpriteComponent(const tinyxml2::XMLElement& componentElement, Entity& entity, std::string& errorMessage) {
    TextureAsset* textureAsset = getTextureAsset(componentElement);
    if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
        std::ostringstream stream;
        stream << "Prefab sprite texture is missing: "
               << (componentElement.Attribute("texture") == nullptr ? "<none>" : componentElement.Attribute("texture"));
        errorMessage = stream.str();
        return false;
    }

    entity.addComponent<SpriteComponent>(makeSpriteFromAttributes(componentElement, *textureAsset));
    return true;
}

bool loadAnimationComponent(const tinyxml2::XMLElement& componentElement, Entity& entity, std::string& errorMessage) {
    Animation animation(componentElement.FloatAttribute("frameDuration", 0.1f));

    for (const tinyxml2::XMLElement* frame = componentElement.FirstChildElement("frame");
         frame != nullptr;
         frame = frame->NextSiblingElement("frame")) {
        TextureAsset* textureAsset = getTextureAsset(*frame);
        if (textureAsset == nullptr || textureAsset->getTextureHandle() == nullptr) {
            std::ostringstream stream;
            stream << "Prefab animation frame texture is missing: "
                   << (frame->Attribute("texture") == nullptr ? "<none>" : frame->Attribute("texture"));
            errorMessage = stream.str();
            return false;
        }

        animation.addFrame(makeSpriteFromAttributes(*frame, *textureAsset));
    }

    if (!animation.hasFrames()) {
        errorMessage = "Prefab animation component has no frames.";
        return false;
    }

    AnimationComponent& animationComponent = entity.addComponent<AnimationComponent>(animation);
    if (!parseBool(componentElement.Attribute("playing"), true)) {
        animationComponent.pause();
    }

    return true;
}
}

bool PrefabSerializer::saveEntity(const Entity& entity, const std::string& path, std::string& errorMessage) {
    namespace fs = std::filesystem;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* prefab = document.NewElement("prefab");
    prefab->SetAttribute("version", 1);
    prefab->SetAttribute("name", entity.getName().c_str());
    document.InsertEndChild(prefab);

    tinyxml2::XMLElement* entityElement = document.NewElement("entity");
    entityElement->SetAttribute("name", entity.getName().c_str());
    entityElement->SetAttribute("tag", entity.getTag().c_str());
    prefab->InsertEndChild(entityElement);

    if (const TransformComponent* transform = entity.getComponent<TransformComponent>()) {
        saveTransform(document, *entityElement, *transform);
    }

    if (const SpriteComponent* spriteComponent = entity.getComponent<SpriteComponent>()) {
        saveSprite(document, *entityElement, *spriteComponent);
    }

    if (const AnimationComponent* animationComponent = entity.getComponent<AnimationComponent>()) {
        saveAnimation(document, *entityElement, *animationComponent);
    }

    if (const CollisionComponent* collisionComponent = entity.getComponent<CollisionComponent>()) {
        saveCollision(document, *entityElement, *collisionComponent);
    }

    if (const ScriptComponent* scriptComponent = entity.getComponent<ScriptComponent>()) {
        saveScript(document, *entityElement, *scriptComponent);
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

    const fs::path outputPath(path);
    std::error_code directoryError;
    if (!outputPath.parent_path().empty()) {
        fs::create_directories(outputPath.parent_path(), directoryError);
        if (directoryError) {
            errorMessage = "Failed to create prefab directory: " + directoryError.message();
            return false;
        }
    }

    const tinyxml2::XMLError result = document.SaveFile(path.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to save prefab: " + std::string(document.ErrorStr());
        return false;
    }

    errorMessage.clear();
    return true;
}

Entity* PrefabSerializer::instantiate(
    const std::string& path,
    Level& level,
    const Vector2F& position,
    std::string& errorMessage
) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(path.c_str()) != tinyxml2::XML_SUCCESS) {
        errorMessage = "Failed to load prefab: " + std::string(document.ErrorStr());
        return nullptr;
    }

    const tinyxml2::XMLElement* prefab = document.FirstChildElement("prefab");
    const tinyxml2::XMLElement* entityElement = prefab == nullptr ? nullptr : prefab->FirstChildElement("entity");
    if (entityElement == nullptr) {
        errorMessage = "Prefab is missing an entity element.";
        return nullptr;
    }

    Entity& entity = level.createEntity();
    entity.setName(entityElement->Attribute("name") == nullptr ? "PrefabEntity" : entityElement->Attribute("name"));
    entity.setTag(entityElement->Attribute("tag") == nullptr ? "Prefab" : entityElement->Attribute("tag"));

    float rotation = 0.0f;
    for (const tinyxml2::XMLElement* component = entityElement->FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type != nullptr && std::string(type) == "TransformComponent") {
            rotation = component->FloatAttribute("rotation", 0.0f);
            break;
        }
    }

    entity.addComponent<TransformComponent>(position, rotation);

    for (const tinyxml2::XMLElement* component = entityElement->FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type == nullptr) {
            continue;
        }

        const std::string componentType(type);
        if (componentType == "TransformComponent") {
            continue;
        } else if (componentType == "SpriteComponent") {
            if (!loadSpriteComponent(*component, entity, errorMessage)) {
                level.destroyEntity(entity);
                return nullptr;
            }
        } else if (componentType == "AnimationComponent") {
            if (!loadAnimationComponent(*component, entity, errorMessage)) {
                level.destroyEntity(entity);
                return nullptr;
            }
        } else if (componentType == "CollisionComponent") {
            const float width = component->FloatAttribute("width", 1.0f);
            const float height = component->FloatAttribute("height", 1.0f);
            const bool isSensor = parseBool(component->Attribute("isSensor"), false);
            const char* colliderName = component->Attribute("name");
            entity.addComponent<CollisionComponent>(
                width,
                height,
                bodyTypeFromString(component->Attribute("bodyType")),
                isSensor,
                colliderName == nullptr ? "Collider" : colliderName
            );
        } else if (componentType == "ScriptComponent") {
            const char* scriptPath = component->Attribute("path");
            if (scriptPath != nullptr && scriptPath[0] != '\0') {
                entity.addComponent<ScriptComponent>(scriptPath);
            }
        } else if (componentType == "PlayerController") {
            entity.addComponent<PlayerController>();
        } else if (componentType == "Brick") {
            entity.addComponent<Brick>();
        } else if (componentType == "Bullet") {
            entity.addComponent<Bullet>();
        }
    }

    entity.addInputListeners(InputSystem::getInstance());
    errorMessage.clear();
    return &entity;
}
