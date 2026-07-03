#pragma once

#include <functional>
#include <string>

class Entity;
class Level;

class EntityTreePanel {
public:
    using SelectEntityCallback = std::function<void(Entity&, bool)>;
    using SyncEntityCallback = std::function<void(Entity&, bool)>;
    using DrawEntityDetailsCallback = std::function<void(Entity&, std::string&)>;
    using DeleteEntityCallback = std::function<void(int)>;

    void draw(
        Level& level,
        const std::string& levelLabel,
        int selectedEntityId,
        SelectEntityCallback selectEntityCallback,
        SyncEntityCallback syncEntityCallback,
        DrawEntityDetailsCallback drawEntityDetailsCallback,
        DeleteEntityCallback deleteEntityCallback,
        std::string& statusMessage
    );

private:
    void drawEntityTreeNode(
        Level& level,
        Entity& entity,
        int selectedEntityId,
        SelectEntityCallback& selectEntityCallback,
        SyncEntityCallback& syncEntityCallback,
        DrawEntityDetailsCallback& drawEntityDetailsCallback,
        DeleteEntityCallback& deleteEntityCallback,
        std::string& statusMessage
    );
};
