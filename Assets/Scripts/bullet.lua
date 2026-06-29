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
}}

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
        debugMoveTimer = 0.0
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

    local position = transform:getPosition()
    local direction = self:getForwardDirection(transform:getRotation())

    position.x = position.x + direction.x * self.speed * deltaTime
    position.y = position.y + direction.y * self.speed * deltaTime

    transform:setPosition(position)

    if self:shouldDebug() then
        self.debugMoveTimer = self.debugMoveTimer - deltaTime
        if self.debugMoveTimer <= 0.0 then
            self.debugMoveTimer = self.debugMoveLogInterval
            Engine.log("Bullet update " .. self:getDebugName() .. " pos=(" .. tostring(position.x) .. ", " ..
                           tostring(position.y) .. ") rot=" .. tostring(transform:getRotation()) .. " enabled=" ..
                           tostring(self.entity:isEnabled()) .. " active=" .. tostring(self.active) .. " speed=" ..
                           tostring(self.speed) .. " dt=" .. tostring(deltaTime))
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
end

function onStart(entity, script)
    local bullet = getBulletState(entity)
    applyScriptProperties(bullet, script)

    entity:setTag("Bullet")
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

function clearOwner(entity)
    local bullet = getBulletState(entity)
    if bullet ~= nil then
        if bullet:shouldDebug() then
            Engine.log("Bullet owner cleared: " .. bullet:getDebugName())
        end

        bullet.owner = nil
        bullet.active = false
    end
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

function onCollisionEnter(entity, otherEntity, selfCollider, otherCollider, normal)
    local bullet = getBulletState(entity)

    if entity == nil or bullet == nil or not entity:isEnabled() then
        return
    end

    if bullet.active == false then
        return
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
            Engine.log("Bullet ignored owner collision " .. bullet:getDebugName() .. " owner=" ..
                           bullet:getOwnerDebugName() .. " hit=" .. tostring(otherName) .. "#" ..
                           tostring(otherEntity:getId()))
        end

        return
    end

    local normalizedTag = string.lower(otherTag or "")
    if normalizedTag ~= "player" and normalizedTag ~= "obstacle" and normalizedTag ~= "enemy" then
        if bullet:shouldDebug() then
            Engine.log(
                "Bullet ignored collision " .. bullet:getDebugName() .. " owner=" .. bullet:getOwnerDebugName() ..
                    " hit=" .. tostring(otherName) .. " tag=" .. tostring(otherTag) .. " reason=unhandled_tag")
        end

        return
    end

    Engine.log("Bullet handled collision " .. bullet:getDebugName() .. " owner=" .. bullet:getOwnerDebugName() ..
                   " hit=" .. tostring(otherName) .. " tag=" .. tostring(otherTag) .. " active=" ..
                   tostring(bullet.active))

    bullet.active = false

    if normalizedTag == "enemy" and otherEntity ~= nil then
        otherEntity:callScript("takeDamage", otherEntity, bullet.damage)
    end

    if normalizedTag == "obstacle" and otherEntity ~= nil then
        local boxNormal = {
            x = -normal.x,
            y = -normal.y
        }
        local hitSide = getHitSide(boxNormal)

        Engine.log(
            "Bullet hit obstacle " .. bullet:getDebugName() .. " owner=" .. bullet:getOwnerDebugName() .. " hit=" ..
                tostring(otherName) .. " side=" .. tostring(hitSide))
    end

    local manager = Engine.findEntityByName("BulletManager")
    if manager ~= nil and manager:callScript("resetBullet", entity) then
        return
    end

    entity:setEnabled(false)
end
