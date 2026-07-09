#include "components/CollisionComponent.h"

#include "Entity.h"
#include "components/TransformComponent.h"
#include "math/Math2D.h"
#include "system/PhysicsSystem.h"
#include "system/ScriptSystem.h"

#include <iostream>
#include <utility>

CollisionComponent::CollisionComponent(float width, float height)
    : CollisionComponent(width, height, BodyType::Static, false, "Collider") {}

CollisionComponent::CollisionComponent(
    float width,
    float height,
    BodyType bodyType,
    bool isSensor,
    std::string name,
    float rotation
)
    : width(width),
      height(height),
      offset(Vector2F::zero()),
      rotation(rotation),
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

    b2BodyDef bodyDef = b2DefaultBodyDef();
    bodyDef.type = toBox2DBodyType(bodyType);
    const Vector2F worldPosition = transform->getWorldPosition();
    bodyDef.position = {worldPosition.x, worldPosition.y};
    bodyDef.rotation = b2MakeRot(
        transform->getWorldRotation() * Math2D::DegreesToRadians
    );
    bodyDef.fixedRotation = fixedRotation;
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

    const b2Polygon box = b2MakeOffsetBox(
        width * 0.5f,
        height * 0.5f,
        b2Vec2{offset.x, offset.y},
        b2MakeRot(rotation * Math2D::DegreesToRadians)
    );
    shapeId = b2CreatePolygonShape(bodyId, &shapeDef, &box);
    if (!b2Shape_IsValid(shapeId)) {
        std::cerr << "Failed to create Box2D shape for CollisionComponent." << std::endl;
    }
}

void CollisionComponent::onUpdate(float deltaTime) {
    if (!this->isEnabled()) {
        return;
    }

    (void)deltaTime;
    if (bodyType == BodyType::Static) {
        syncBodyToTransform();
        return;
    }

    syncTransformToBody();
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

Vector2F CollisionComponent::getWorldPosition() const {
    if (b2Body_IsValid(bodyId)) {
        const b2Vec2 position = b2Body_GetPosition(bodyId);
        const float bodyRotation =
            b2Rot_GetAngle(b2Body_GetRotation(bodyId))
            / Math2D::DegreesToRadians;
        return Vector2F(position.x, position.y)
            + Math2D::rotate(offset, bodyRotation);
    }

    const Entity* entity = getEntity();
    if (entity == nullptr) {
        return Vector2F::zero();
    }

    const TransformComponent* transform = entity->getComponent<TransformComponent>();
    if (transform == nullptr) {
        return Vector2F::zero();
    }

    return transform->getWorldPosition()
        + Math2D::rotate(offset, transform->getWorldRotation());
}

Vector2F CollisionComponent::getLinearVelocity() const {
    if (!b2Body_IsValid(bodyId)) {
        return Vector2F::zero();
    }

    const b2Vec2 velocity = b2Body_GetLinearVelocity(bodyId);
    return Vector2F(velocity.x, velocity.y);
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

float CollisionComponent::getRotation() const {
    return rotation;
}

float CollisionComponent::getWorldRotation() const {
    if (b2Body_IsValid(bodyId)) {
        return b2Rot_GetAngle(b2Body_GetRotation(bodyId))
            / Math2D::DegreesToRadians
            + rotation;
    }

    const Entity* entity = getEntity();
    const TransformComponent* transform =
        entity == nullptr ? nullptr : entity->getComponent<TransformComponent>();
    return transform == nullptr
        ? rotation
        : transform->getWorldRotation() + rotation;
}

CollisionComponent::BodyType CollisionComponent::getBodyType() const {
    return bodyType;
}

bool CollisionComponent::isSensor() const {
    return sensor;
}

bool CollisionComponent::isFixedRotation() const {
    return fixedRotation;
}

void CollisionComponent::setName(const std::string& name) {
    this->name = name;
}

void CollisionComponent::setOffset(const Vector2F& offset) {
    this->offset = offset;
    updateShapeGeometry();
}

void CollisionComponent::setSize(float width, float height) {
    if (width <= 0.0f || height <= 0.0f) {
        std::cerr << "CollisionComponent size must be greater than zero." << std::endl;
        return;
    }

    this->width = width;
    this->height = height;
    updateShapeGeometry();
}

void CollisionComponent::setRotation(float value) {
    rotation = value;
    updateShapeGeometry();
}

void CollisionComponent::setBodyType(BodyType bodyType) {
    this->bodyType = bodyType;
    rebuildBody();
}

void CollisionComponent::setSensor(bool sensor) {
    this->sensor = sensor;
    rebuildBody();
}

void CollisionComponent::setLinearVelocity(const Vector2F& velocity) {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    b2Body_SetLinearVelocity(bodyId, b2Vec2{velocity.x, velocity.y});
}

void CollisionComponent::setFixedRotation(bool value) {
    fixedRotation = value;

    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    b2Body_SetFixedRotation(bodyId, value);
}

void CollisionComponent::setListener(CollisionListener* listener) {
    this->listener = listener;
}

void CollisionComponent::notifyCollisionEnter(CollisionComponent& other, const Vector2F& normal, const Vector2F& contactPoint) {
    if (listener != nullptr) {
        listener->onCollisionEnter(*this, other, normal, contactPoint);
    }

    Entity* owner = getEntity();
    if (owner == nullptr || owner->isDestroyed() || !owner->isEnabled()) {
        return;
    }

    ScriptSystem::getInstance().tryCallEntityCollisionFunction(
        *owner,
        "onCollisionEnter",
        *this,
        other,
        normal,
        contactPoint
    );
}


void CollisionComponent::notifySensorEnter(CollisionComponent &other) {
    if (listener != nullptr) {
        listener->onSensorEnter(*this, other);
    }

    Entity *owner = getEntity();
    if (owner == nullptr || owner->isDestroyed() || !owner->isEnabled()) {
        return;
    }

    ScriptSystem::getInstance().tryCallEntitySensorFunction(
        *owner,
        "onSensorEnter",
        *this,
        other
    );
}

void CollisionComponent::syncBodyToTransform() {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    TransformComponent* transform = getEntity()->getComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    const Vector2F worldPosition = transform->getWorldPosition();
    b2Body_SetTransform(
        bodyId,
        {worldPosition.x, worldPosition.y},
        b2MakeRot(transform->getWorldRotation() * Math2D::DegreesToRadians)
    );
}

void CollisionComponent::syncTransformToBody() {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    Entity* entity = getEntity();
    if (entity == nullptr) {
        return;
    }

    TransformComponent* transform = entity->getComponent<TransformComponent>();
    if (transform == nullptr) {
        return;
    }

    const b2Vec2 bodyPosition = b2Body_GetPosition(bodyId);
    const float bodyRotation =
        b2Rot_GetAngle(b2Body_GetRotation(bodyId))
        / Math2D::DegreesToRadians;
    transform->setWorldPosition(Vector2F(bodyPosition.x, bodyPosition.y));
    transform->setWorldRotation(bodyRotation);
}

std::unique_ptr<Component> CollisionComponent::clone() const {
    auto copy = std::make_unique<CollisionComponent>(
        width,
        height,
        bodyType,
        sensor,
        name,
        rotation
    );
    copy->setOffset(offset);
    copy->setFixedRotation(fixedRotation);
    copy->setEnabled(isEnabled());
    return copy;
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

void CollisionComponent::updateShapeGeometry() {
    if (!b2Shape_IsValid(shapeId)) {
        return;
    }

    const b2Polygon box = b2MakeOffsetBox(
        width * 0.5f,
        height * 0.5f,
        b2Vec2{offset.x, offset.y},
        b2MakeRot(rotation * Math2D::DegreesToRadians)
    );
    b2Shape_SetPolygon(shapeId, &box);
}

void CollisionComponent::rebuildBody() {
    if (!b2Body_IsValid(bodyId)) {
        return;
    }

    onDestroy();
    onStart();
}
