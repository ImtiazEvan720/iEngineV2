#pragma once

#include "Entity.h"

#include <cstddef>
#include <deque>
#include <string>

class Level {
public:
    Level() = default;
    ~Level() = default;

    Level(const Level& other) = delete;
    Level& operator=(const Level& other) = delete;
    Level(Level&& other) noexcept = default;
    Level& operator=(Level&& other) noexcept = default;

    static Level& getCurrentLevel();
    static Level createEmpty();
    static Level& createNewLevel();
    static bool saveCurrentLevel(std::string& errorMessage);
    static bool saveCurrentLevel(const std::string& path, std::string& errorMessage);
    static bool loadFromFile(const std::string& path, Level& level, std::string& errorMessage);
    static bool loadCurrentLevel(std::string& errorMessage);
    static void loadLevel(Level&& level, bool keepPersistentEntities = false);
    static void startPendingComponents();
    static const std::string& getCurrentLevelPath();
    static void setCurrentLevelPath(const std::string& path);

    Entity& createEntity();
    Entity& addEntity(Entity entity);
    bool removeEntity(std::size_t index);
    bool destroyEntity(Entity& entity);
    bool destroyEntity(Entity* entity);
    bool destroyEntityById(int id);
    bool destroyEntityHierarchy(Entity& entity);
    bool destroyEntityHierarchy(Entity* entity);
    bool destroyEntityByIdHierarchy(int id);
    void cleanupDestroyedEntities();
    void clearEntities();

    Entity* getEntity(std::size_t index);
    const Entity* getEntity(std::size_t index) const;

    std::deque<Entity>& getEntities();
    const std::deque<Entity>& getEntities() const;
    const std::string& getSourcePath() const;
    std::string getSourceFileName() const;

    void update(float deltaTime);

private:
    std::deque<Entity> entities;
    std::string sourcePath;
    std::deque<Entity> extractPersistentEntities();
};
