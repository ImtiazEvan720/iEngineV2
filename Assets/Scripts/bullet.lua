ScriptProperties = {
    { name = "speed", type = "float", default = 300.0 },
}

local Bullet = {}
Bullet.__index = Bullet

function Bullet:new(entity)
    return setmetatable({
        entity = entity,
        speed = 300.0,
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

function onStart(entity, script)
    bullet = Bullet:new(entity)

    if script ~= nil then
        bullet.speed = script:getFloat("speed", bullet.speed)
    end

    entity:setTag("Bullet")
end

function onUpdate(entity, deltaTime, script)
    if bullet == nil then
        bullet = Bullet:new(entity)
    end

    if script ~= nil then
        bullet.speed = script:getFloat("speed", bullet.speed)
    end

    bullet:update(deltaTime)
end
