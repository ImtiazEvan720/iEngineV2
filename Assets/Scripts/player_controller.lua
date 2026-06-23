ScriptProperties = {
    { name = "speed", type = "float", default = 100.0 },
    { name = "bulletManager", type = "string", default = "BulletManager" },
}

local speed = 100.0
local bulletManagerName = "BulletManager"
local bulletManager = nil

local function refreshScriptProperties(script)
    if script == nil then
        return
    end

    speed = script:getFloat("speed", speed)
    bulletManagerName = script:getString("bulletManager", bulletManagerName)
end

local function getBulletManager()
    if bulletManager ~= nil and bulletManager:getName() == bulletManagerName then
        return bulletManager
    end

    bulletManager = Engine.findEntityByName(bulletManagerName)
    return bulletManager
end

local function fire(entity, transform)
    local position = transform:getPosition()
    local rotation = transform:getRotation()
    local manager = getBulletManager()

    if manager == nil then
        engineLog("PlayerControllerLua could not find bullet manager: " .. bulletManagerName)
        return
    end

    if not manager:callScript("fire", position, rotation) then
        engineLog("PlayerControllerLua failed to call fire on manager: " .. bulletManagerName)
    end
end

function onStart(entity, script)
    refreshScriptProperties(script)
    bulletManager = Engine.findEntityByName(bulletManagerName)

    engineLog("PlayerControllerLua started: "
        .. entity:getName()
        .. ", tag: "
        .. entity:getTag())
end

function onUpdate(entity, deltaTime, script)
    refreshScriptProperties(script)

    local transform = entity:getTransform()
    if transform == nil then
        return
    end

    if Input.wasActionPressed("Fire") then
        fire(entity, transform)
    end

    local position = transform:getPosition()
    local rotation = transform:getRotation()
    local moving = false

    if Input.isActionDown("MoveUp") then
        position.y = position.y - speed * deltaTime
        rotation = 0.0
        moving = true
    elseif Input.isActionDown("MoveDown") then
        position.y = position.y + speed * deltaTime
        rotation = 180.0
        moving = true
    elseif Input.isActionDown("MoveLeft") then
        position.x = position.x - speed * deltaTime
        rotation = -90.0
        moving = true
    elseif Input.isActionDown("MoveRight") then
        position.x = position.x + speed * deltaTime
        rotation = 90.0
        moving = true
    end

    local animation = entity:getAnimation()
    if moving then
        transform:setPosition(position)
        transform:setRotation(rotation)

        if animation ~= nil then
            animation:play()
        end
    elseif animation ~= nil then
        animation:pause()
    end
end
