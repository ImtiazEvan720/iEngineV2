#include "components/CollisionComponent.h"

#include "Entity.h"
#include "components/TransformComponent.h"
#include "system/PhysicsSystem.h"

#include <cmath>
#include <iostream>
#include <utility>

CollisionComponent::CollisionComponent(float width, float height)
    : CollisionComponent(width, height, BodyType::Static, false, "Collider") {}

CollisionComponent::CollisionComponent(float width, float height, BodyType bodyType, bool isSensor, std::string name)
    : width(width),
      height(height),
      offset(Vector2F::zero()),
      bodyType(bodyType),
      sensor(isSensor),
      name(std::move(name)) {}

void CollisionComponent::onStart() {
    PhysicsSystem& physicsSystem = PhysicsSystem::getInstance();
    if (!physicsSystem.isInitialized()) {
        physicsSystem.initialize();
    }

    TransformComponent* transform = getEntity()->getComponent<TransformComponent>();
    if (transform == nullptr) {
        std::cerr << "CollisionComponent requires a TransformComponent." << std::endl;
        return;
    }

    constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = toBox2DBodyType(bodyType);
    const Vector2F worldPosition = transform->getWorldPosition();
    bodyDef.position = {worldPosition.x + offset.x, worldPosition.y + offset.y};
    bodyDef.rotation = b2MakeRot(transform->getWorldRotation() * degreesToRadians);
    bodyDef.userData = this;

    bodyId = b2CreateBody(physicsSystem.getWorldId(), &bodyDef);
    if (!b2Body_IsValid(bodyId)) {
        std::cerr << "Failed to create Box2D body for CollisionComponent." << std::endl;
        return;
    }

    b2ShapeDef shapeDef = b2DefaultShapeDef();
    shapeDef.userData = this;
    shapeDef.isSensor = sensor;
    shapeDef.enableSensorEvents = true;
    shapeDef.enableContactEvents = true;
    shapeDef.density = 1.0f;
    shapeDef.material.friction = 0.3f;

    const b2Polygon box = b2MakeBox(width * 0.5f, height * 0.5f);
    shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    if (!b2Shape_IsValid(shapeId)) {
        std::cerr << "Failed to create Box2D shape for CollisionComponent." << std::endl;
    }
}

void CollisionComponent::onUpdate(float deltaTime) {
    (void)deltaTime;
    syncBodyToTransform();
}

void CollisionComponent::onEnable(bool value) {
    if (!value) {
        onDestroy();
        return;
    }

    if (!b2Body_IsValid(bodyId)) {
        onStart();
        return;
    }

    syncBodyToTransform();
}

void CollisionComponent::onDestroy() {
    if (b2Body_IsValid(bodyId)) {
        b2DestroyBody(bodyId);
        bodyId = b2_nullBodyId;
        shapeId = b2_nullShapeId;
    }
}

b2BodyId CollisionComponent::getBodyId() const {
    return bodyId;
}

b2ShapeId CollisionComponent::getShapeId() const {
    return shapeId;
}

const std::string& CollisionComponent::getName() const {
    return name;
}

const Vector2F& CollisionComponent::getOffset() const {
    return offset;
}

float CollisionComponent::getWidth() const {
    return width;
}

float CollisionComponent::getHeight() const {
    return height;
}

CollisionComponent::BodyType CollisionComponent::getBodyType() const {
    return bodyType;
}

bool CollisionComponent::isSensor() const {
    return sensor;
}

void CollisionComponent::setName(const std::string& name) {
    this->name = name;
}

void CollisionComponent::setOffset(const Vector2F& offset) {
    this->offset = offset;
    syncBodyToTransform();
}

void CollisionComponent::setSize(float width, float height) {
    if (width <= 0.0f || height <= 0.0f) {
        std::cerr << "CollisionComponent size must be greater than zero." << std::endl;
        return;
    }

    this->width = width;
    this->height = height;
    rebuildBody();
}

void CollisionComponent::setBodyType(BodyType bodyType) {
    this->bodyType = bodyType;
    rebuildBody();
}

void CollisionComponent::setSensor(bool sensor) {
    this->sensor = sensor;
    rebuildBody();
}

void CollisionComponent::setListener(CollisionListener* listener) {
    this->listener = listener;
}

void CollisionComponent::notifyCollisionEnter(CollisionComponent& other) {
    if (listener != nullptr) {
        listener->onCollisionEnter(*this, other);
    }
}

void CollisionComponent::syncBodyToTransform() {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    TransformComponent* transform = getEntity()->getComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    constexpr float degreesToRadians = 3.14159265358979323846f / 180.0f;
    const Vector2F worldPosition = transform->getWorldPosition();
    b2Body_SetTransform(
        bodyId,
        {worldPosition.x + offset.x, worldPosition.y + offset.y},
        b2MakeRot(transform->getWorldRotation() * degreesToRadians)
    );
}

b2BodyType CollisionComponent::toBox2DBodyType(BodyType bodyType) {
    switch (bodyType) {
        case BodyType::Kinematic:
            return b2_kinematicBody;
        case BodyType::Dynamic:
            return b2_dynamicBody;
        case BodyType::Static:
        default:
            return b2_staticBody;
    }
}

void CollisionComponent::rebuildBody() {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    onDestroy();
    onStart();
}
