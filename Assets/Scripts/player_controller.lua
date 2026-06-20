local speed = 100.0
local firedBullets = 0
local bulletPrefabName = "Bullet"

local function fire(entity, transform)
    local position = transform:getPosition()
    local rotation = transform:getRotation()
    local bullet = spawnPrefab(bulletPrefabName, position.x, position.y, rotation)

    if bullet == nil then
        engineLog("PlayerControllerLua failed to spawn prefab: " .. bulletPrefabName)
        return
    end

    bullet:setName("Bullet" .. tostring(firedBullets))
    bullet:setTag("Bullet")
    firedBullets = firedBullets + 1
end

function onStart(entity)
    engineLog("PlayerControllerLua started: "
        .. entity:getName()
        .. ", tag: "
        .. entity:getTag())
end

function onUpdate(entity, deltaTime)
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
