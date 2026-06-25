ScriptProperties = {
    { name = "speed", type = "float", default = 100.0 },
    { name = "obstacleTag", type = "string", default = "Obstacle" },
    { name = "obstacleRayDistance", type = "float", default = 32.0 },
    { name = "bulletManager", type = "entity", default = "" },
    { name = "turret", type = "entity", default = "" },
}

local speed = 100.0
local obstacleTag = "Obstacle"
local obstacleRayDistance = 32.0
local bulletManagerName = "BulletManager"
local bulletManager = nil
local turret = nil

local function refreshScriptProperties(script)
    if Props ~= nil and Props.speed ~= nil then
        speed = Props.speed
    end

    if Props ~= nil and Props.obstacleTag ~= nil then
        obstacleTag = Props.obstacleTag
    end

    if Props ~= nil and Props.obstacleRayDistance ~= nil then
        obstacleRayDistance = Props.obstacleRayDistance
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

    if Props == nil or Props.obstacleTag == nil then
        obstacleTag = script:getString("obstacleTag", obstacleTag)
    end

    if Props == nil or Props.obstacleRayDistance == nil then
        obstacleRayDistance = script:getFloat("obstacleRayDistance", obstacleRayDistance)
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

local function isBlocked(transform, moveX, moveY, moveDistance)
    local worldPosition = transform:getWorldPosition()
    local rayDistance = moveDistance + obstacleRayDistance
    local rayEnd = Vector2F.new(
        worldPosition.x + moveX * rayDistance,
        worldPosition.y + moveY * rayDistance
    )

    local hit = Engine.raycast(worldPosition, rayEnd)
    if hit == nil or not hit.hit then
        return false
    end

    return string.lower(hit.tag or "") == string.lower(obstacleTag or "Obstacle")
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
    local moveX = 0.0
    local moveY = 0.0
    local moving = false

    if Input.isActionDown("MoveUp") then
        moveY = -1.0
        rotation = 0.0
        moving = true
    elseif Input.isActionDown("MoveDown") then
        moveY = 1.0
        rotation = 180.0
        moving = true
    elseif Input.isActionDown("MoveLeft") then
        moveX = -1.0
        rotation = -90.0
        moving = true
    elseif Input.isActionDown("MoveRight") then
        moveX = 1.0
        rotation = 90.0
        moving = true
    end

    local animation = entity:getAnimation()
    if moving then
        local moveDistance = speed * deltaTime
        if isBlocked(transform, moveX, moveY, moveDistance) then
            transform:setRotation(rotation)

            if animation ~= nil then
                animation:pause()
            end

            return
        end

        position.x = position.x + moveX * moveDistance
        position.y = position.y + moveY * moveDistance

        transform:setRotation(rotation)
        transform:setPosition(position)

        if animation ~= nil then
            animation:play()
        end
    elseif animation ~= nil then
        animation:pause()
    end
end
