#include "serialization/PrefabSerializer.h"

#include "Entity.h"
#include "misc/Level.h"
#include "serialization/ComponentSerializerRegistry.h"
#include "system/InputSystem.h"

#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <vector>

namespace {
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
            if (left->getEditorDisplayOrder() != right->getEditorDisplayOrder()) {
                return left->getEditorDisplayOrder() < right->getEditorDisplayOrder();
            }

            return left->getId() < right->getId();
        }
    );

    return children;
}

void saveEntityRecursive(
    tinyxml2::XMLDocument& document,
    tinyxml2::XMLElement& parentElement,
    const Entity& entity
) {
    tinyxml2::XMLElement* entityElement = document.NewElement("entity");
    entityElement->SetAttribute("name", entity.getName().c_str());
    entityElement->SetAttribute("tag", entity.getTag().c_str());
    entityElement->SetAttribute("enabled", entity.isEnabled());
    entityElement->SetAttribute("displayOrder", entity.getEditorDisplayOrder());
    parentElement.InsertEndChild(entityElement);

    ComponentSerializerRegistry::getInstance().saveComponents(document, *entityElement, entity);

    const std::vector<const Entity*> children = getSortedPrefabChildren(entity);
    if (children.empty()) {
        return;
    }

    tinyxml2::XMLElement* childrenElement = document.NewElement("children");
    entityElement->InsertEndChild(childrenElement);

    for (const Entity* child : children) {
        saveEntityRecursive(document, *childrenElement, *child);
    }
}

Entity* instantiateEntityRecursive(
    const tinyxml2::XMLElement& entityElement,
    Level& level,
    Entity* parent,
    const Vector2F& rootPosition,
    bool isRootEntity,
    std::vector<Entity*>& createdEntities,
    std::string& errorMessage
) {
    Entity& entity = level.createEntity();
    createdEntities.push_back(&entity);

    entity.setName(entityElement.Attribute("name") == nullptr ? "PrefabEntity" : entityElement.Attribute("name"));
    entity.setTag(entityElement.Attribute("tag") == nullptr ? "Prefab" : entityElement.Attribute("tag"));
    entity.setEnabled(entityElement.BoolAttribute("enabled", true));
    entity.setEditorDisplayOrder(entityElement.IntAttribute("displayOrder", entity.getEditorDisplayOrder()));

    if (parent != nullptr && !entity.setParent(parent, false)) {
        errorMessage = "Failed to attach prefab child to parent entity.";
        return nullptr;
    }

    ComponentSerializationContext context;
    context.isRootEntity = isRootEntity;
    context.rootPosition = rootPosition;

    if (!ComponentSerializerRegistry::getInstance().loadComponents(entityElement, entity, context, errorMessage)) {
        return nullptr;
    }

    entity.addInputListeners(InputSystem::getInstance());

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
            createdEntities,
            errorMessage
        );
        if (child == nullptr) {
            return nullptr;
        }
    }

    return &entity;
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

    saveEntityRecursive(document, *prefab, entity);

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

    std::vector<Entity*> createdEntities;
    Entity* entity = instantiateEntityRecursive(
        *entityElement,
        level,
        nullptr,
        position,
        true,
        createdEntities,
        errorMessage
    );
    if (entity == nullptr) {
        for (auto iterator = createdEntities.rbegin(); iterator != createdEntities.rend(); ++iterator) {
            level.destroyEntity(*iterator);
        }
        level.cleanupDestroyedEntities();
        return nullptr;
    }

    errorMessage.clear();
    return entity;
}
