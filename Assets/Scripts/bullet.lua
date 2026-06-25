ScriptProperties = {
    { name = "speed", type = "float", default = 300.0 },
    { name = "damage", type = "float", default = 1.0 },
}

local Bullet = {}
Bullet.__index = Bullet

function Bullet:new(entity)
    return setmetatable({
        entity = entity,
        speed = 300.0,
        damage = 1.0,
        owner = nil,
    }, Bullet)
end

function Bullet:getForwardDirection(rotation)
    local radians = math.rad(rotation)

    return Vector2F.new(
        math.sin(radians),
        -math.cos(radians)
    )
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
end

local bullet = nil
local pendingOwner = nil

local function ensureBullet(entity)
    if bullet == nil then
        bullet = Bullet:new(entity)
        bullet.owner = pendingOwner
    elseif bullet.entity == nil and entity ~= nil then
        bullet.entity = entity
    end
end

function onStart(entity, script)
    bullet = Bullet:new(entity)

    if script ~= nil then
        bullet.speed = script:getFloat("speed", bullet.speed)
        bullet.damage = script:getFloat("damage", bullet.damage)
    end

    entity:setTag("Bullet")
end

function onUpdate(entity, deltaTime, script)
    ensureBullet(entity)

    if script ~= nil then
        bullet.speed = script:getFloat("speed", bullet.speed)
        bullet.damage = script:getFloat("damage", bullet.damage)
    end

    bullet:update(deltaTime)
end

function setOwner(ownerEntity)
    pendingOwner = ownerEntity
    if bullet ~= nil then
        bullet.owner = ownerEntity
    end
end

function clearOwner()
    pendingOwner = nil
    if bullet ~= nil then
        bullet.owner = nil
    end
end

function onCollisionEnter(entity, otherEntity, selfCollider, otherCollider)
    ensureBullet(entity)

    local otherName = "unknown"
    local otherTag = ""
    if otherEntity ~= nil then
        otherName = otherEntity:getName()
        otherTag = otherEntity:getTag()
    elseif otherCollider ~= nil then
        otherName = otherCollider:getName()
    end

    Engine.log("Bullet hit " .. tostring(otherName))

    if bullet.owner ~= nil and otherEntity ~= nil and bullet.owner:getId() == otherEntity:getId() then
        return
    end

    local normalizedTag = string.lower(otherTag or "")
    if normalizedTag ~= "player" and normalizedTag ~= "obstacle" and normalizedTag ~= "enemy" then
        return
    end

    if normalizedTag == "enemy" and otherEntity ~= nil then
        otherEntity:callScript("takeDamage", bullet.damage)
    end

    local manager = Engine.findEntityByName("BulletManager")
    if manager ~= nil and manager:callScript("resetBullet", entity) then
        return
    end

    entity:setEnabled(false)
end
