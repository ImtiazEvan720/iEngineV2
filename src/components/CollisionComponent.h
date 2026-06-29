#pragma once

#include "components/Component.h"
#include "math/Vector2F.h"

#include <box2d/box2d.h>

#include <string>

class CollisionComponent;

class CollisionListener {
public:
    virtual ~CollisionListener() = default;
    virtual void onCollisionEnter(
        CollisionComponent& self,
        CollisionComponent& other,
        const Vector2F& normal
    ) = 0;
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
    void onEnable(bool value) override;
    void onDestroy() override;

    b2BodyId getBodyId() const;
    b2ShapeId getShapeId() const;
    Vector2F getWorldPosition() const;
    const std::string& getName() const;
    const Vector2F& getOffset() const;
    float getWidth() const;
    float getHeight() const;
    BodyType getBodyType() const;
    bool isSensor() const;
    void setName(const std::string& name);
    void setOffset(const Vector2F& offset);
    void setSize(float width, float height);
    void setBodyType(BodyType bodyType);
    void setSensor(bool sensor);
    void setListener(CollisionListener* listener);
    void notifyCollisionEnter(CollisionComponent& other, const Vector2F& normal);
    void syncBodyToTransform();
    std::unique_ptr<Component> clone() const override;

private:
    static b2BodyType toBox2DBodyType(BodyType bodyType);
    void rebuildBody();

    float width;
    float height;
    Vector2F offset;
    BodyType bodyType;
    bool sensor;
    std::string name;
    b2BodyId bodyId = b2_nullBodyId;
    b2ShapeId shapeId = b2_nullShapeId;
    CollisionListener* listener = nullptr;
};
