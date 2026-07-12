ScriptProperties = {{
    name = "speed",
    type = "float",
    default = 100.0
}, {
    name = "bulletManager",
    type = "entity",
    default = ""
}, {
    name = "turret",
    type = "entity",
    default = ""
}}

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

    if not manager:sendEvent("Fire", {
        position = position,
        rotation = rotation,
        owner = entity
    }) then
        engineLog("PlayerControllerLua failed to call fire on manager: " .. bulletManagerName)
    end
end

local function setColliderVelocity(collider, x, y)
    if collider == nil then
        return
    end

    local currentVelocity = collider:getLinearVelocity()
    if currentVelocity ~= nil and math.abs(currentVelocity.x - x) < 0.001 and math.abs(currentVelocity.y - y) < 0.001 then
        return
    end

    collider:setLinearVelocity(Vector2F.new(x, y))
end

function onStart(entity, script)
    refreshScriptProperties(script)
    entity:setTag("Player")

    if bulletManager == nil then
        bulletManager = Engine.findEntityByName(bulletManagerName)
    end

    local collider = entity:getCollision()
    if collider ~= nil then
        collider:setFixedRotation(true)
    end

    engineLog("PlayerControllerLua started: " .. entity:getName() .. ", tag: " .. entity:getTag())

    local animation = entity:getAnimation()
    if animation ~= nil then
        engineLog("PlayerControllerLua found animation: ")
    else
        engineLog("PlayerControllerLua found no animation.")
    end
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

    local rotation = transform:getRotation()
    local moveX = 0.0
    local moveY = 0.0
    local moving = false
    local move = Input.getAxis2D("Move")

    if move ~= nil and move.y < -0.001 then
        moveY = -1.0
        rotation = 0.0
        moving = true
    elseif move ~= nil and move.y > 0.001 then
        moveY = 1.0
        rotation = 180.0
        moving = true
    elseif move ~= nil and move.x < -0.001 then
        moveX = -1.0
        rotation = -90.0
        moving = true
    elseif move ~= nil and move.x > 0.001 then
        moveX = 1.0
        rotation = 90.0
        moving = true
    end

    local collider = entity:getCollision()
    local animation = entity:getAnimation()
    if moving then
        transform:setRotation(rotation)
        setColliderVelocity(collider, moveX * speed, moveY * speed)
        if animation ~= nil then
            animation:play()
        end
    else
        setColliderVelocity(collider, 0.0, 0.0)

        if animation ~= nil then
            animation:pause()
        end
    end
end
