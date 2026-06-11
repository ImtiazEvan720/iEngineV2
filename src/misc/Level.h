#ifndef IENGINEV2_LEVEL_H
#define IENGINEV2_LEVEL_H

#include "Entity.h"
#include "misc/LevelAsset.h"

#include <cstddef>
#include <deque>

class Level {
public:
    ~Level() = default;

    Level(const Level& other) = delete;
    Level& operator=(const Level& other) = delete;
    Level(Level&& other) = delete;
    Level& operator=(Level&& other) = delete;

    static Level& getCurrentLevel();

    Entity& createEntity();
    Entity& addEntity(Entity entity);
    bool removeEntity(std::size_t index);
    bool destroyEntity(Entity& entity);
    bool destroyEntity(Entity* entity);
    bool destroyEntityById(int id);
    void cleanupDestroyedEntities();
    void clearEntities();

    Entity* getEntity(std::size_t index);
    const Entity* getEntity(std::size_t index) const;

    std::deque<Entity>& getEntities();
    const std::deque<Entity>& getEntities() const;

    bool loadFromAsset(LevelAsset& levelAsset);
    void update(float deltaTime);

private:
    Level() = default;

    std::deque<Entity> entities;
};

#endif
