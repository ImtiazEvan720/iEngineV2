#ifndef IENGINEV2_PREFABSERIALIZER_H
#define IENGINEV2_PREFABSERIALIZER_H

#include "math/Vector2F.h"

#include <string>

class Entity;
class Level;

class PrefabSerializer {
public:
    static bool saveEntity(const Entity& entity, const std::string& path, std::string& errorMessage);
    static Entity* instantiate(const std::string& path, Level& level, const Vector2F& position, std::string& errorMessage);
};

#endif
