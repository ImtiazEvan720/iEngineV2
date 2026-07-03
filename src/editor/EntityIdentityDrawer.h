#pragma once

#include <string>

class Entity;

class EntityIdentityDrawer {
public:
    void clear();
    void syncFromEntity(Entity& entity, bool force = false);
    void draw(Entity& entity, std::string& statusMessage);

private:
    struct EditState {
        int entityId = -1;
        std::string name;
        std::string tag;
        bool enabled = true;
    };

    EditState editState;
};
