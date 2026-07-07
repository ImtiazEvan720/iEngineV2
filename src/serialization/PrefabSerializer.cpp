#include "serialization/PrefabSerializer.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "misc/Level.h"
#include "serialization/ComponentSerializerRegistry.h"
#include "serialization/components/ScriptComponentSerializer.h"
#include "system/InputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace {
using EntityPrefabIdMap = std::unordered_map<const Entity*, std::string>;
using PrefabIdEntityMap = std::unordered_map<std::string, Entity*>;

void collectPrefabEntityIdsRecursive(
    const Entity& entity,
    EntityPrefabIdMap& entityToPrefabId,
    ScriptComponentSerializer::PrefabLocalGuidMap& guidToPrefabId,
    int& nextPrefabId
) {
    const std::string prefabId = "entity_" + std::to_string(nextPrefabId);
    ++nextPrefabId;

    entityToPrefabId[&entity] = prefabId;
    guidToPrefabId[entity.getGuid()] = prefabId;

    for (const Entity* child : entity.getChildren()) {
        if (child != nullptr && !child->isDestroyed()) {
            collectPrefabEntityIdsRecursive(*child, entityToPrefabId, guidToPrefabId, nextPrefabId);
        }
    }
}

std::vector<const Entity*> getSortedPrefabChildren(const Entity& entity) {
    std::vector<const Entity*> children;
    for (const Entity* child : entity.getChildren()) {
        if (child != nullptr && !child->isDestroyed()) {
            children.push_back(child);
        }
    }

    std::sort(
        children.begin(),
        children.end(),
        [](const Entity* left, const Entity* right) {
            if (left->getDisplayOrder() != right->getDisplayOrder()) {
                return left->getDisplayOrder() < right->getDisplayOrder();
            }

            return left->getId() < right->getId();
        }
    );

    return children;
}

void saveEntityRecursive(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& parentElement,
    const Entity& entity,
    const EntityPrefabIdMap& entityToPrefabId
) {
    tinyxml2::XMLElement* entityElement = document.NewElement("entity");
    const auto prefabIdIterator = entityToPrefabId.find(&entity);
    if (prefabIdIterator != entityToPrefabId.end()) {
        entityElement->SetAttribute("prefabId", prefabIdIterator->second.c_str());
    }
    entityElement->SetAttribute("name", entity.getName().c_str());
    entityElement->SetAttribute("tag", entity.getTag().c_str());
    entityElement->SetAttribute("enabled", entity.isEnabled());
    entityElement->SetAttribute("displayOrder", entity.getDisplayOrder());
    parentElement.InsertEndChild(entityElement);

    ComponentSerializerRegistry::getInstance().saveComponents(document, *entityElement, entity);

    const std::vector<const Entity*> children = getSortedPrefabChildren(entity);
    if (children.empty()) {
        return;
    }

    tinyxml2::XMLElement* childrenElement = document.NewElement("children");
    entityElement->InsertEndChild(childrenElement);

    for (const Entity* child : children) {
        saveEntityRecursive(document, *childrenElement, *child, entityToPrefabId);
    }
}

void resolvePrefabLocalScriptValue(ScriptValue& value, const PrefabIdEntityMap& prefabIdToEntity) {
    if (value.type != ScriptValueType::Entity
        || value.entityReferenceScope != ScriptEntityReferenceScope::PrefabLocal) {
        return;
    }

    const auto iterator = prefabIdToEntity.find(value.stringValue);
    if (iterator == prefabIdToEntity.end() || iterator->second == nullptr) {
        value.stringValue.clear();
    } else {
        value.stringValue = iterator->second->getGuid();
    }

    value.entityReferenceScope = ScriptEntityReferenceScope::Level;
}

void resolvePrefabLocalScriptProperty(ScriptProperty& property, const PrefabIdEntityMap& prefabIdToEntity) {
    if (property.type == ScriptPropertyType::Entity
        && property.entityReferenceScope == ScriptEntityReferenceScope::PrefabLocal) {
        const auto iterator = prefabIdToEntity.find(property.stringValue);
        if (iterator == prefabIdToEntity.end() || iterator->second == nullptr) {
            property.stringValue.clear();
        } else {
            property.stringValue = iterator->second->getGuid();
        }

        property.entityReferenceScope = ScriptEntityReferenceScope::Level;
        return;
    }

    if (property.type == ScriptPropertyType::Array && property.elementType == ScriptValueType::Entity) {
        for (ScriptValue& value : property.arrayValue) {
            resolvePrefabLocalScriptValue(value, prefabIdToEntity);
        }
        return;
    }

    if (property.type == ScriptPropertyType::Map && property.mapValueType == ScriptValueType::Entity) {
        for (ScriptMapEntry& entry : property.mapValue) {
            resolvePrefabLocalScriptValue(entry.value, prefabIdToEntity);
        }
    }
}

void resolvePrefabLocalScriptReferences(
    const std::vector<Entity*>& createdEntities,
    const PrefabIdEntityMap& prefabIdToEntity
) {
    for (Entity* entity : createdEntities) {
        if (entity == nullptr) {
            continue;
        }

        ScriptComponent* scriptComponent = entity->getComponent<ScriptComponent>();
        if (scriptComponent == nullptr) {
            continue;
        }

        for (ScriptProperty& property : scriptComponent->getProperties()) {
            resolvePrefabLocalScriptProperty(property, prefabIdToEntity);
        }
    }
}

Entity* instantiateEntityRecursive(
    const tinyxml2::XMLElement& entityElement,
    Level& level,
    Entity* parent,
    const Vector2F& rootPosition,
    bool isRootEntity,
    ComponentSerializationPlacement placement,
    std::vector<Entity*>& createdEntities,
    PrefabIdEntityMap& prefabIdToEntity,
    std::string& errorMessage
) {
    Entity& entity = level.createEntity();
    entity.setComponentStartupDeferred(true);
    createdEntities.push_back(&entity);

    entity.setName(entityElement.Attribute("name") == nullptr ? "PrefabEntity" : entityElement.Attribute("name"));
    entity.setTag(entityElement.Attribute("tag") == nullptr ? "Prefab" : entityElement.Attribute("tag"));
    entity.setEnabled(entityElement.BoolAttribute("enabled", true));
    entity.setDisplayOrder(entityElement.IntAttribute("displayOrder", entity.getDisplayOrder()));

    const char* prefabId = entityElement.Attribute("prefabId");
    if (prefabId != nullptr && prefabId[0] != '\0') {
        prefabIdToEntity[prefabId] = &entity;
    }

    if (parent != nullptr && !entity.setParent(parent, false)) {
        errorMessage = "Failed to attach prefab child to parent entity.";
        return nullptr;
    }

    ComponentSerializationContext context;
    context.isRootEntity = isRootEntity;
    context.rootPosition = rootPosition;
    context.placement = placement;

    if (!ComponentSerializerRegistry::getInstance().loadComponents(entityElement, entity, context, errorMessage)) {
        return nullptr;
    }

    const tinyxml2::XMLElement* childrenElement = entityElement.FirstChildElement("children");
    for (const tinyxml2::XMLElement* childElement =
             childrenElement == nullptr ? nullptr : childrenElement->FirstChildElement("entity");
         childElement != nullptr;
         childElement = childElement->NextSiblingElement("entity")) {
        Entity* child = instantiateEntityRecursive(
            *childElement,
            level,
            &entity,
            rootPosition,
            false,
            placement,
            createdEntities,
            prefabIdToEntity,
            errorMessage
        );
        if (child == nullptr) {
            return nullptr;
        }
    }

    return &entity;
}

void startCreatedPrefabEntities(const std::vector<Entity*>& createdEntities) {
    InputSystem& inputSystem = InputSystem::getInstance();
    for (Entity* entity : createdEntities) {
        if (entity == nullptr) {
            continue;
        }

        entity->startDeferredComponents();
        entity->addInputListeners(inputSystem);
    }
}
}

bool PrefabSerializer::saveEntity(const Entity& entity, const std::string& path, std::string& errorMessage) {
    namespace fs = std::filesystem;

    tinyxml2::XMLDocument document;
    document.InsertEndChild(document.NewDeclaration(R"(xml version="1.0" encoding="UTF-8")"));

    tinyxml2::XMLElement* prefab = document.NewElement("prefab");
    prefab->SetAttribute("version", 2);
    prefab->SetAttribute("name", entity.getName().c_str());
    document.InsertEndChild(prefab);

    EntityPrefabIdMap entityToPrefabId;
    ScriptComponentSerializer::PrefabLocalGuidMap guidToPrefabId;
    int nextPrefabId = 0;
    collectPrefabEntityIdsRecursive(entity, entityToPrefabId, guidToPrefabId, nextPrefabId);

    ScriptComponentSerializer::setPrefabLocalGuidMap(&guidToPrefabId);
    saveEntityRecursive(document, *prefab, entity, entityToPrefabId);
    ScriptComponentSerializer::setPrefabLocalGuidMap(nullptr);

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
    std::string& errorMessage,
    ComponentSerializationPlacement placement
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

    std::vector<Entity*> createdEntities;
    PrefabIdEntityMap prefabIdToEntity;
    Entity* entity = instantiateEntityRecursive(
        *entityElement,
        level,
        nullptr,
        position,
        true,
        placement,
        createdEntities,
        prefabIdToEntity,
        errorMessage
    );
    if (entity == nullptr) {
        for (auto iterator = createdEntities.rbegin(); iterator != createdEntities.rend(); ++iterator) {
            level.destroyEntity(*iterator);
        }
        level.cleanupDestroyedEntities();
        return nullptr;
    }

    resolvePrefabLocalScriptReferences(createdEntities, prefabIdToEntity);
    startCreatedPrefabEntities(createdEntities);

    errorMessage.clear();
    return entity;
}
