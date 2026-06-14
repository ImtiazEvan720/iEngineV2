#include "misc/Level.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/PlayerController.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "game/Brick.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
std::string trim(const std::string& value) {
    std::size_t begin = 0;
    while (begin < value.size() && std::isspace(static_cast<unsigned char>(value[begin]))) {
        ++begin;
    }

    std::size_t end = value.size();
    while (end > begin && std::isspace(static_cast<unsigned char>(value[end - 1]))) {
        --end;
    }

    return value.substr(begin, end - begin);
}

std::vector<std::string> splitComponents(const std::string& value) {
    std::vector<std::string> components;
    std::stringstream stream(value);
    std::string token;

    while (std::getline(stream, token, ',')) {
        token = trim(token);
        if (!token.empty()) {
            components.push_back(token);
        }
    }

    return components;
}

std::string getStringProperty(
    const std::unordered_map<std::string, std::string>& properties,
    const std::string& name,
    const std::string& fallback = ""
) {
    const auto iterator = properties.find(name);
    return iterator == properties.end() ? fallback : iterator->second;
}

float getFloatProperty(
    const std::unordered_map<std::string, std::string>& properties,
    const std::string& name,
    float fallback
) {
    const auto iterator = properties.find(name);
    if (iterator == properties.end()) {
        return fallback;
    }

    try {
        return std::stof(iterator->second);
    } catch (const std::exception&) {
        return fallback;
    }
}

bool getBoolProperty(
    const std::unordered_map<std::string, std::string>& properties,
    const std::string& name,
    bool fallback
) {
    const auto iterator = properties.find(name);
    if (iterator == properties.end()) {
        return fallback;
    }

    const std::string value = iterator->second;
    return value == "true" || value == "1" || value == "True" || value == "TRUE";
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

const TilesetInfo* findTilesetForGid(const LevelAsset& levelAsset, int gid) {
    const TilesetInfo* result = nullptr;

    for (const TilesetInfo& tileset : levelAsset.getTilesets()) {
        if (gid >= tileset.firstGid) {
            result = &tileset;
        }
    }

    return result;
}

Sprite createSpriteFromLocalTileId(const TilesetInfo& tileset, int localTileId) {
    const float sourceX = static_cast<float>((localTileId % tileset.columns) * tileset.tileWidth);
    const float sourceY = static_cast<float>((localTileId / tileset.columns) * tileset.tileHeight);

    return Sprite(
        tileset.textureAsset->getTextureHandle(),
        RenderRect{
            sourceX,
            sourceY,
            static_cast<float>(tileset.tileWidth),
            static_cast<float>(tileset.tileHeight)
        }
    );
}

int getGidUnderObject(const ObjectInfo& object, const TileLayerInfo& tileLayer, const LevelAsset& levelAsset) {
    if (levelAsset.getTileWidth() == 0 || levelAsset.getTileHeight() == 0) {
        return 0;
    }

    const int tileX = static_cast<int>(std::floor(object.x / static_cast<float>(levelAsset.getTileWidth())));
    const int tileY = static_cast<int>(std::floor(object.y / static_cast<float>(levelAsset.getTileHeight())));

    if (tileX < 0 || tileY < 0 || tileX >= tileLayer.width || tileY >= tileLayer.height) {
        return 0;
    }

    const int index = tileY * tileLayer.width + tileX;
    if (index < 0 || static_cast<std::size_t>(index) >= tileLayer.gids.size()) {
        return 0;
    }

    return tileLayer.gids[static_cast<std::size_t>(index)];
}

void addVisualFromGid(Entity& entity, const ObjectInfo& object, const LevelAsset& levelAsset, int gid) {
    if (gid == 0) {
        return;
    }

    const TilesetInfo* tileset = findTilesetForGid(levelAsset, gid);
    if (tileset == nullptr || tileset->textureAsset == nullptr || tileset->textureAsset->getTextureHandle() == nullptr) {
        return;
    }

    const int localTileId = gid - tileset->firstGid;
    const auto animationIterator = tileset->animations.find(localTileId);

    if (animationIterator != tileset->animations.end()) {
        Animation animation;

        for (const TileAnimationFrame& frame : animationIterator->second.frames) {
            Sprite frameSprite = createSpriteFromLocalTileId(*tileset, frame.tileId);
            frameSprite.setSize(Vector2F(object.width, object.height));
            animation.addFrame(frameSprite);
        }

        if (!animationIterator->second.frames.empty()) {
            animation.setFrameDuration(animationIterator->second.frames.front().durationSeconds);
        }

        entity.addComponent<AnimationComponent>(animation);
        return;
    }

    Sprite sprite = createSpriteFromLocalTileId(*tileset, localTileId);
    sprite.setSize(Vector2F(object.width, object.height));
    entity.addComponent<SpriteComponent>(sprite);
}

void addComponentsFromProperties(Entity& entity, const ObjectInfo& object) {
    const std::string components = getStringProperty(object.properties, "components");

    for (const std::string& componentName : splitComponents(components)) {
        if (componentName == "CollisionComponent") {
            const float width = getFloatProperty(object.properties, "CollisionComponent_width", object.width);
            const float height = getFloatProperty(object.properties, "CollisionComponent_height", object.height);
            const bool isSensor = getBoolProperty(object.properties, "CollisionComponent_isSensor", false);
            const std::string colliderName = getStringProperty(object.properties, "CollisionComponent_name", object.name);
            const CollisionComponent::BodyType bodyType = parseBodyType(
                getStringProperty(object.properties, "CollisionComponent_bodyType", "Static")
            );

            entity.addComponent<CollisionComponent>(width, height, bodyType, isSensor, colliderName);
        } else if (componentName == "Brick") {
            entity.addComponent<Brick>();
        } else if (componentName == "PlayerController") {
            entity.addComponent<PlayerController>();
        } else {
            std::cerr << "Unknown level component: " << componentName
                      << " on object " << object.name << std::endl;
        }
    }
}
}

Level& Level::getCurrentLevel() {
    static Level currentLevel;
    return currentLevel;
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

bool Level::loadFromAsset(LevelAsset& levelAsset) {
    if (!levelAsset.isLoaded() && !levelAsset.load()) {
        return false;
    }

    std::unordered_map<std::string, Entity*> entitiesByName;

    for (const LevelGroupInfo& group : levelAsset.getGroups()) {
        const TileLayerInfo* tileLayer = group.tileLayers.empty() ? nullptr : &group.tileLayers.front();

        for (const ObjectInfo& object : group.objects) {
            Entity& entity = createEntity();
            entity.setName(object.name);
            entity.setTag(object.type.empty() ? object.name : object.type);

            entity.addComponent<TransformComponent>(Vector2F(object.x, object.y), object.rotation);

            if (tileLayer != nullptr) {
                addVisualFromGid(entity, object, levelAsset, getGidUnderObject(object, *tileLayer, levelAsset));
            }

            addComponentsFromProperties(entity, object);

            if (!object.name.empty()) {
                entitiesByName[object.name] = &entity;
            }
        }
    }

    for (const LevelGroupInfo& group : levelAsset.getGroups()) {
        for (const ObjectInfo& object : group.objects) {
            const std::string parentName = getStringProperty(object.properties, "parent");
            if (parentName.empty() || object.name.empty()) {
                continue;
            }

            const auto childIterator = entitiesByName.find(object.name);
            const auto parentIterator = entitiesByName.find(parentName);
            if (childIterator == entitiesByName.end() || parentIterator == entitiesByName.end()) {
                std::cerr << "Failed to connect parent '" << parentName
                          << "' for child '" << object.name << "'" << std::endl;
                continue;
            }

            TransformComponent* childTransform = childIterator->second->getComponent<TransformComponent>();
            TransformComponent* parentTransform = parentIterator->second->getComponent<TransformComponent>();
            if (childTransform != nullptr && parentTransform != nullptr) {
                childTransform->setParent(parentTransform);
            }
        }
    }

    return true;
}

void Level::printEntityPreviewFromAsset(LevelAsset& levelAsset) const {
    if (!levelAsset.isLoaded() && !levelAsset.load()) {
        std::cerr << "Cannot preview level asset entities because the asset failed to load: "
                  << levelAsset.getPath() << std::endl;
        return;
    }

    std::cout << "Level entity preview for " << levelAsset.getName() << std::endl;

    int previewIndex = 0;
    for (const LevelGroupInfo& group : levelAsset.getGroups()) {
        const TileLayerInfo* tileLayer = group.tileLayers.empty() ? nullptr : &group.tileLayers.front();
        std::cout << "  Group \"" << group.name << "\" objects=" << group.objects.size() << std::endl;

        for (const ObjectInfo& object : group.objects) {
            const int gid = tileLayer == nullptr ? 0 : getGidUnderObject(object, *tileLayer, levelAsset);
            const TilesetInfo* tileset = gid == 0 ? nullptr : findTilesetForGid(levelAsset, gid);
            const int localTileId = tileset == nullptr ? 0 : gid - tileset->firstGid;
            const bool animated = tileset != nullptr && tileset->animations.find(localTileId) != tileset->animations.end();
            const std::vector<std::string> customComponents = splitComponents(getStringProperty(object.properties, "components"));
            const std::string parentName = getStringProperty(object.properties, "parent");

            std::cout << "    [" << previewIndex++ << "] Entity name=\""
                      << object.name << "\" tag=\""
                      << (object.type.empty() ? object.name : object.type) << "\"" << std::endl;
            std::cout << "        TransformComponent position=(" << object.x << ", " << object.y
                      << ") rotation=" << object.rotation << std::endl;

            if (gid == 0) {
                std::cout << "        Visual: none, no non-zero tile under object" << std::endl;
            } else if (animated) {
                std::cout << "        AnimationComponent from gid=" << gid
                          << " localTileId=" << localTileId << std::endl;
            } else {
                std::cout << "        SpriteComponent from gid=" << gid
                          << " localTileId=" << localTileId << std::endl;
            }

            if (customComponents.empty()) {
                std::cout << "        Custom components: none" << std::endl;
            } else {
                std::cout << "        Custom components:";
                for (const std::string& component : customComponents) {
                    std::cout << " " << component;
                }
                std::cout << std::endl;
            }

            if (!parentName.empty()) {
                std::cout << "        Parent: " << parentName << std::endl;
            }
        }
    }
}

void Level::update(float deltaTime) {
    for (Entity& entity : entities) {
        if (entity.isDestroyed()) {
            continue;
        }

        entity.update(deltaTime);
    }

    cleanupDestroyedEntities();
}
