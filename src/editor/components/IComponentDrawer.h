#pragma once
#include <string>

class Entity;
class Component;

class IComponentDrawer
{

public:
    virtual ~IComponentDrawer() = default;
    virtual void draw(Entity& entity, Component& component, std::string& statusMessage) = 0;
};