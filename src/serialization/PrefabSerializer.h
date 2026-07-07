#pragma once

#include "math/Vector2F.h"
#include "serialization/ComponentSerializer.h"

#include <string>

class Entity;
class Level;

class PrefabSerializer {
public:
    static bool saveEntity(const Entity& entity, const std::string& path, std::string& errorMessage);
    static Entity* instantiate(
        const std::string& path,
        Level& level,
        const Vector2F& position,
        std::string& errorMessage,
        ComponentSerializationPlacement placement = ComponentSerializationPlacement::World
    );
};
