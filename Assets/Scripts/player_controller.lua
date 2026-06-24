ScriptProperties = {
    { name = "speed", type = "float", default = 100.0 },
    { name = "bulletManager", type = "entity", default = "" },
    { name = "turret", type = "entity", default = "" },
}

local speed = 100.0
local bulletManagerName = "BulletManager"
local bulletManager = nil
local turret = nil

local function refreshScriptProperties(script)
    if Props ~= nil and Props.speed ~= nil then
        speed = Props.speed
    end

    if Refs ~= nil and Refs.bulletManager ~= nil then
        bulletManager = Refs.bulletManager
    end

    if Refs ~= nil and Refs.turret ~= nil then
        turret = Refs.turret
    end

    if script == nil then
        return
    end

    if Props == nil or Props.speed == nil then
        speed = script:getFloat("speed", speed)
    end

    if bulletManager == nil then
        bulletManager = script:getEntity("bulletManager")
    end

    if turret == nil then
        turret = script:getEntity("turret")
    end
end

local function fire(entity, transform)
    local fireTransform = transform
    if turret ~= nil and turret:getTransform() ~= nil then
        fireTransform = turret:getTransform()
    end

    local position = fireTransform:getWorldPosition()
    local rotation = fireTransform:getWorldRotation()
    local manager = bulletManager
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

    if bulletManager == nil then
        bulletManager = Engine.findEntityByName(bulletManagerName)
    end

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
