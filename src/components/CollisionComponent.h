#ifndef IENGINEV2_COLLISIONCOMPONENT_H
#define IENGINEV2_COLLISIONCOMPONENT_H

#include "components/Component.h"

#include <box2d/box2d.h>

#include <string>

class CollisionComponent;

class CollisionListener {
public:
    virtual ~CollisionListener() = default;
    virtual void onCollisionEnter(CollisionComponent& self, CollisionComponent& other) = 0;
};

class CollisionComponent : public Component {
public:
    enum class BodyType {
        Static,
        Kinematic,
        Dynamic
    };

    CollisionComponent(float width, float height);
    CollisionComponent(float width, float height, BodyType bodyType, bool isSensor = false, std::string name = "Collider");
    ~CollisionComponent() override = default;

    void onStart() override;
    void onUpdate(float deltaTime) override;
    void onDestroy() override;

    b2BodyId getBodyId() const;
    b2ShapeId getShapeId() const;
    const std::string& getName() const;
    void setListener(CollisionListener* listener);
    void notifyCollisionEnter(CollisionComponent& other);
    void syncBodyToTransform();

private:
    static b2BodyType toBox2DBodyType(BodyType bodyType);

    float width;
    float height;
    BodyType bodyType;
    bool sensor;
    std::string name;
    b2BodyId bodyId = b2_nullBodyId;
    b2ShapeId shapeId = b2_nullShapeId;
    CollisionListener* listener = nullptr;
};

#endif
