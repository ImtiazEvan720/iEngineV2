#include "system/PhysicsSystem.h"
#include "components/CollisionComponent.h"
#include "components/TransformComponent.h"

#include <cmath>
#include <iostream>

namespace
{
    Vector2F getContactPoint(const b2Manifold &manifold)
    {
        if (manifold.pointCount <= 0)
        {
            return Vector2F::zero();
        }

        Vector2F point = Vector2F::zero();
        for (int i = 0; i < manifold.pointCount; ++i)
        {
            point.x += manifold.points[i].point.x;
            point.y += manifold.points[i].point.y;
        }

        const float count = static_cast<float>(manifold.pointCount);
        return Vector2F(point.x / count, point.y / count);
    }

    void dispatchCollision(b2ShapeId shapeIdA, b2ShapeId shapeIdB, const Vector2F &normalFromAToB, const Vector2F &contactPoint)
    {
        if (!b2Shape_IsValid(shapeIdA) || !b2Shape_IsValid(shapeIdB))
        {
            return;
        }

        auto *colliderA = static_cast<CollisionComponent *>(b2Shape_GetUserData(shapeIdA));
        auto *colliderB = static_cast<CollisionComponent *>(b2Shape_GetUserData(shapeIdB));

        if (colliderA == nullptr || colliderB == nullptr)
        {
            return;
        }

        if (!colliderA->isEnabled() || !colliderB->isEnabled())
        {
            return;
        }

        std::cout << "Collision started between "
                  << colliderA->getName() << " and "
                  << colliderB->getName() << std::endl;

        colliderA->notifyCollisionEnter(*colliderB, normalFromAToB, contactPoint);
        colliderB->notifyCollisionEnter(*colliderA, normalFromAToB * -1.0f, contactPoint);
    }

    void dispatchSensorCollision(b2ShapeId sensorShapeId, b2ShapeId visitorShapeId)
    {
        if (!b2Shape_IsValid(sensorShapeId) || !b2Shape_IsValid(visitorShapeId))
        {
            return;
        }

        auto *sensor = static_cast<CollisionComponent *>(b2Shape_GetUserData(sensorShapeId));
        auto *visitor = static_cast<CollisionComponent *>(b2Shape_GetUserData(visitorShapeId));

        if (sensor == nullptr || visitor == nullptr)
        {
            return;
        }

        if (!sensor->isEnabled() || !visitor->isEnabled())
        {
            return;
        }

        sensor->notifySensorEnter(*visitor);
        visitor->notifySensorEnter(*sensor);
    }
}

    PhysicsSystem &PhysicsSystem::getInstance()
    {
        static PhysicsSystem instance;
        return instance;
    }

    PhysicsSystem::~PhysicsSystem()
    {
        shutdown();
    }

    void PhysicsSystem::initialize(float gravityX, float gravityY)
    {
        if (isInitialized())
        {
            return;
        }

        b2WorldDef worldDef = b2DefaultWorldDef();
        worldDef.gravity = {gravityX, gravityY};
        worldId = b2CreateWorld(&worldDef);
    }

    void PhysicsSystem::update(float deltaTime)
    {
        if (!isInitialized())
        {
            return;
        }

        b2World_Step(worldId, deltaTime, 4);

        processingEvents = true;

        const b2ContactEvents contactEvents = b2World_GetContactEvents(worldId);
        for (int i = 0; i < contactEvents.beginCount; ++i)
        {
            const b2ContactBeginTouchEvent &event = contactEvents.beginEvents[i];
            const Vector2F contactPoint = getContactPoint(event.manifold);
            Vector2F normal(event.manifold.normal.x, event.manifold.normal.y);

            dispatchCollision(event.shapeIdA, event.shapeIdB, normal, contactPoint);
        }

        const b2SensorEvents sensorEvents = b2World_GetSensorEvents(worldId);
        for (int i = 0; i < sensorEvents.beginCount; ++i)
        {
            const b2SensorBeginTouchEvent &event = sensorEvents.beginEvents[i];
            dispatchSensorCollision(event.sensorShapeId, event.visitorShapeId);
        }

        processingEvents = false;
    }

    void PhysicsSystem::shutdown()
    {
        if (!isInitialized())
        {
            return;
        }

        b2DestroyWorld(worldId);
        worldId = b2_nullWorldId;
    }

    b2WorldId PhysicsSystem::getWorldId() const
    {
        return worldId;
    }

    bool PhysicsSystem::isInitialized() const
    {
        return b2World_IsValid(worldId);
    }

    bool PhysicsSystem::isProcessingEvents() const
    {
        return processingEvents;
    }

    PhysicsRaycastHit PhysicsSystem::raycast(const Vector2F &start, const Vector2F &end) const
    {
        PhysicsRaycastHit hit;
        if (!isInitialized())
        {
            return hit;
        }

        if (!std::isfinite(start.x) || !std::isfinite(start.y) || !std::isfinite(end.x) || !std::isfinite(end.y))
        {
            return hit;
        }

        const b2Vec2 origin{start.x, start.y};
        const b2Vec2 translation{
            end.x - start.x,
            end.y - start.y};

        if (std::fabs(translation.x) <= 0.0001f && std::fabs(translation.y) <= 0.0001f)
        {
            return hit;
        }

        const b2RayResult result = b2World_CastRayClosest(
            worldId,
            origin,
            translation,
            b2DefaultQueryFilter());

        hit.hit = result.hit;
        hit.point = Vector2F(result.point.x, result.point.y);
        hit.normal = Vector2F(result.normal.x, result.normal.y);
        hit.fraction = result.fraction;
        hit.nodeVisits = result.nodeVisits;
        hit.leafVisits = result.leafVisits;

        if (result.hit && b2Shape_IsValid(result.shapeId))
        {
            auto *collider = static_cast<CollisionComponent *>(b2Shape_GetUserData(result.shapeId));
            if (collider == nullptr || !collider->isEnabled())
            {
                hit.hit = false;
                hit.collider = nullptr;
                return hit;
            }

            hit.collider = collider;
        }

        return hit;
    }
