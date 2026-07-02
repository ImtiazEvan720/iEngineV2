ScriptProperties = {{
    name = "speed",
    type = "float",
    default = 300.0
}, {
    name = "damage",
    type = "float",
    default = 1.0
}, {
    name = "debugMovement",
    type = "bool",
    default = true
}, {
    name = "debugAllBullets",
    type = "bool",
    default = true
}, {
    name = "debugBulletName",
    type = "string",
    default = "PooledBullet1"
}, {
    name = "debugMoveLogInterval",
    type = "float",
    default = 0.25
},
{
    name = "SparkEffectPrefab",
    type = "prefab",
    default = "SparkEffect"
}
}

local Bullet = {}
Bullet.__index = Bullet

function Bullet:new(entity)
    return setmetatable({
        entity = entity,
        speed = 300.0,
        damage = 1.0,
        owner = nil,
        active = false,
        debugMovement = true,
        debugAllBullets = true,
        debugBulletName = "PooledBullet1",
        debugMoveLogInterval = 0.25,
        debugMoveTimer = 0.0,
        sparkEffectPrefab = "SparkEffect"
    }, Bullet)
end

function Bullet:getForwardDirection(rotation)
    local radians = math.rad(rotation)

    return Vector2F.new(math.sin(radians), -math.cos(radians))
end

function Bullet:shouldDebug()
    if not self.debugMovement or self.entity == nil then
        return false
    end

    return self.debugAllBullets or self.entity:getName() == self.debugBulletName
end

function Bullet:getDebugName()
    if self.entity == nil then
        return "nil"
    end

    return self.entity:getName() .. "#" .. tostring(self.entity:getId())
end

function Bullet:getOwnerDebugName()
    if self.owner == nil then
        return "nil"
    end

    return self.owner:getName() .. "#" .. tostring(self.owner:getId())
end

function Bullet:update(deltaTime)
    if self.entity == nil or not self.entity:isEnabled() then
        return
    end

    local transform = self.entity:getTransform()
    if transform == nil then
        return
    end

    if self:shouldDebug() then
        self.debugMoveTimer = self.debugMoveTimer - deltaTime
        if self.debugMoveTimer <= 0.0 then
            self.debugMoveTimer = self.debugMoveLogInterval
            local position = transform:getPosition()
            local collider = self.entity:getCollision()
            local velocity = collider == nil and Vector2F.new(0.0, 0.0) or collider:getLinearVelocity()
            Engine.log("Bullet update " .. self:getDebugName() .. " pos=(" .. tostring(position.x) .. ", " ..
                           tostring(position.y) .. ") rot=" .. tostring(transform:getRotation()) .. " enabled=" ..
                           tostring(self.entity:isEnabled()) .. " active=" .. tostring(self.active) .. " speed=" ..
                           tostring(self.speed) .. " velocity=(" .. tostring(velocity.x) .. ", " ..
                           tostring(velocity.y) .. ") dt=" .. tostring(deltaTime))
        end
    end
end

local bulletStates = rawget(_G, "__iEngineBulletStates")
if bulletStates == nil then
    bulletStates = {}
    rawset(_G, "__iEngineBulletStates", bulletStates)
end

local function getBulletState(entity)
    if entity == nil then
        return nil
    end

    local entityId = entity:getId()
    local state = bulletStates[entityId]
    if state == nil then
        state = Bullet:new(entity)
        bulletStates[entityId] = state
    else
        state.entity = entity
    end

    return state
end

local function applyScriptProperties(bullet, script)
    if bullet == nil or script == nil then
        return
    end

    bullet.speed = script:getFloat("speed", bullet.speed)
    bullet.damage = script:getFloat("damage", bullet.damage)
    bullet.debugMovement = script:getBool("debugMovement", bullet.debugMovement)
    bullet.debugAllBullets = script:getBool("debugAllBullets", bullet.debugAllBullets)
    bullet.debugBulletName = script:getString("debugBulletName", bullet.debugBulletName)
    bullet.debugMoveLogInterval = script:getFloat("debugMoveLogInterval", bullet.debugMoveLogInterval)
    bullet.sparkEffectPrefab = script:getString("SparkEffectPrefab", bullet.sparkEffectPrefab)
end

function onStart(entity, script)
    local bullet = getBulletState(entity)
    applyScriptProperties(bullet, script)

    entity:setTag("Bullet")

    local collider = entity:getCollision()
    if collider ~= nil then
        collider:setFixedRotation(true)
        collider:setLinearVelocity(Vector2F.new(0.0, 0.0))
    end
end

function onUpdate(entity, deltaTime, script)
    local bullet = getBulletState(entity)
    applyScriptProperties(bullet, script)

    if bullet.debugMoveLogInterval <= 0.0 then
        bullet.debugMoveLogInterval = 0.25
    end

    bullet:update(deltaTime)
end

function setOwner(entity, ownerEntity)
    local bullet = getBulletState(entity)
    if bullet ~= nil then
        bullet.owner = ownerEntity
        bullet.active = true
        bullet.debugMoveTimer = 0.0

        if bullet:shouldDebug() then
            local ownerName = ownerEntity == nil and "nil" or ownerEntity:getName()
            Engine.log("Bullet owner set: " .. bullet:getDebugName() .. " owner=" .. tostring(ownerName))
        end
    end
end

function fireBullet(entity, event)
    local bullet = getBulletState(entity)
    if bullet == nil or event == nil then
        return false
    end

    if event.owner ~= nil then
        bullet.owner = event.owner
    else
        bullet.owner = nil
    end

    bullet.active = true
    bullet.debugMoveTimer = 0.0

    local direction = event.direction
    if direction == nil then
        local rotation = event.rotation or 0.0
        direction = bullet:getForwardDirection(rotation)
    end

    local collider = entity:getCollision()
    if collider ~= nil then
        collider:setFixedRotation(true)
        collider:setLinearVelocity(Vector2F.new(
            direction.x * bullet.speed,
            direction.y * bullet.speed
        ))
    end

    if bullet:shouldDebug() then
        local ownerName = bullet.owner == nil and "nil" or bullet.owner:getName()
        Engine.log("Bullet fired: "
            .. bullet:getDebugName()
            .. " owner="
            .. tostring(ownerName)
            .. " velocity=("
            .. tostring(direction.x * bullet.speed)
            .. ", "
            .. tostring(direction.y * bullet.speed)
            .. ")")
    end

    return true
end

function clearOwner(entity)
    local bullet = getBulletState(entity)
    if bullet ~= nil then
        if bullet:shouldDebug() then
            Engine.log("Bullet owner cleared: " .. bullet:getDebugName())
        end

        bullet.owner = nil
        bullet.active = false
    end

    local collider = entity:getCollision()
    if collider ~= nil then
        collider:setLinearVelocity(Vector2F.new(0.0, 0.0))
    end
end

function onEvent(entity, event)
    if event == nil then
        return false
    end

    if event.type == "SetOwner" then
        setOwner(entity, event.owner)
        return true
    end

    if event.type == "FireBullet" then
        return fireBullet(entity, event)
    end

    if event.type == "ClearOwner" then
        clearOwner(entity)
        return true
    end

    return false
end

function getOtherDirection(normal)
    if math.abs(normal.x) > math.abs(normal.y) then
        return normal.x > 0 and "Right" or "Left"
    end

    return normal.y > 0 and "Bottom" or "Top"
end

function getHitSide(normal)
    local direction = getOtherDirection(normal)

    if direction == "Right" then
        return "RightSide"
    end
    if direction == "Left" then
        return "LeftSide"
    end
    if direction == "Bottom" then
        return "BottomSide"
    end
    return "TopSide"
end

local function getEntityPosition(entity)
    if entity == nil then
        return nil
    end

    local transform = entity:getTransform()
    if transform == nil then
        return nil
    end

    return transform:getPosition()
end

local function approximateNormalFromEntities(entity, otherEntity)
    local selfPosition = getEntityPosition(entity)
    local otherPosition = getEntityPosition(otherEntity)

    if selfPosition == nil or otherPosition == nil then
        return Vector2F.new(0.0, 0.0)
    end

    local deltaX = otherPosition.x - selfPosition.x
    local deltaY = otherPosition.y - selfPosition.y

    if math.abs(deltaX) > math.abs(deltaY) then
        return Vector2F.new(deltaX > 0.0 and 1.0 or -1.0, 0.0)
    end

    return Vector2F.new(0.0, deltaY > 0.0 and 1.0 or -1.0)
end

local function handleBulletHit(entity, otherEntity, selfCollider, otherCollider, normal, contactPoint, eventName)
    local bullet = getBulletState(entity)

    if entity == nil or bullet == nil or not entity:isEnabled() then
        return
    end

    if bullet.active == false then
        return
    end

    if normal == nil then
        normal = Vector2F.new(0.0, 0.0)
    end

    if contactPoint == nil then
        contactPoint = getEntityPosition(entity)
    end

    local otherName = "unknown"
    local otherTag = ""
    if otherEntity ~= nil then
        otherName = otherEntity:getName()
        otherTag = otherEntity:getTag()
    elseif otherCollider ~= nil then
        otherName = otherCollider:getName()
    end

    if bullet.owner ~= nil and otherEntity ~= nil and bullet.owner:getId() == otherEntity:getId() then
        if bullet:shouldDebug() then
            Engine.log("Bullet ignored owner " .. tostring(eventName) .. " " .. bullet:getDebugName() .. " owner=" ..
                           bullet:getOwnerDebugName() .. " hit=" .. tostring(otherName) .. "#" ..
                           tostring(otherEntity:getId()))
        end

        return
    end

    local normalizedTag = string.lower(otherTag or "")
    if normalizedTag ~= "player" and normalizedTag ~= "obstacle" and normalizedTag ~= "enemy" then
        if bullet:shouldDebug() then
            Engine.log("Bullet ignored " .. tostring(eventName) .. " " .. bullet:getDebugName() .. " owner=" ..
                           bullet:getOwnerDebugName() .. " hit=" .. tostring(otherName) .. " tag=" .. tostring(otherTag) ..
                           " reason=unhandled_tag")
        end

        return
    end

    Engine.log("Bullet handled " .. tostring(eventName) .. " " .. bullet:getDebugName() .. " owner=" ..
                   bullet:getOwnerDebugName() .. " hit=" .. tostring(otherName) .. " tag=" .. tostring(otherTag) ..
                   " active=" .. tostring(bullet.active))

    bullet.active = false

    if normalizedTag == "enemy" and otherEntity ~= nil then
        local damageEvent = {
            source = entity,
            damage = bullet.damage,
            contactPoint = contactPoint,
            normal = normal
        }

        if otherEntity:sendEvent("Damage", damageEvent) then
            Engine.log("Enemy life = " .. tostring(damageEvent.life or 0.0))
            Engine.debugDrawPoint(contactPoint, 6.0, 255, 255, 0, 255, 0.25)
        else
            Engine.log("Enemy damage event was not handled by " .. tostring(otherName))
        end
    end

    if normalizedTag == "obstacle" and otherEntity ~= nil then
        local boxNormal = {
            x = -normal.x,
            y = -normal.y
        }
        local hitSide = getHitSide(boxNormal)

        if contactPoint ~= nil then
            -- Engine.debugDrawPoint(contactPoint, 6.0, 255, 255, 0, 255, 0.25)
            local effect = spawnPrefab(bullet.sparkEffectPrefab, contactPoint, 0.0)
            if effect == nil then
                Engine.log("Failed to spawn " .. tostring(bullet.sparkEffectPrefab) .. " prefab.")
            end
        end

        Engine.log(
            "Bullet hit obstacle " .. bullet:getDebugName() .. " owner=" .. bullet:getOwnerDebugName() .. " hit=" ..
                tostring(otherName) .. " side=" .. tostring(hitSide))

    end

    local manager = Engine.findEntityByName("BulletManager")
    if manager ~= nil and manager:sendEvent("ResetBullet", { bullet = entity }) then
        return
    end

    entity:setEnabled(false)
end

function onCollisionEnter(entity, otherEntity, selfCollider, otherCollider, normal, contactPoint)
    handleBulletHit(entity, otherEntity, selfCollider, otherCollider, normal, contactPoint, "collision")
end

function onSensorEnter(entity, otherEntity, selfCollider, otherCollider)
    local normal = approximateNormalFromEntities(entity, otherEntity)
    local contactPoint = getEntityPosition(entity)

    handleBulletHit(entity, otherEntity, selfCollider, otherCollider, normal, contactPoint, "sensor")
end
