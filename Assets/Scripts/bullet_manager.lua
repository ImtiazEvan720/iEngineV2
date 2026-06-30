ScriptProperties = {
    { name = "bulletPrefab", type = "prefab", default = "Bullet" },
    { name = "poolSize", type = "int", default = 50 },
    { name = "spawnOffset", type = "float", default = 24.0 },
    { name = "debugBullets", type = "bool", default = true },
    { name = "debugAllBullets", type = "bool", default = true },
    { name = "debugBulletName", type = "string", default = "PooledBullet1" },
}

local BulletManager = {}
BulletManager.__index = BulletManager

function BulletManager:new(ownerEntity)
    return setmetatable({
        ownerEntity = ownerEntity,
        bulletPrefabName = "Bullet",
        poolSize = 50,
        spawnOffset = 24.0,
        debugBullets = true,
        debugAllBullets = true,
        debugBulletName = "PooledBullet1",
        bullets = {},
    }, BulletManager)
end

local function formatPosition(position)
    if position == nil then
        return "(nil)"
    end

    return "(" .. tostring(position.x) .. ", " .. tostring(position.y) .. ")"
end

function BulletManager:getForwardDirection(rotation)
    local radians = math.rad(rotation)

    return Vector2F.new(
        math.sin(radians),
        -math.cos(radians)
    )
end

function BulletManager:shouldDebugBullet(bulletEntity)
    if not self.debugBullets or bulletEntity == nil then
        return false
    end

    return self.debugAllBullets or bulletEntity:getName() == self.debugBulletName
end

function BulletManager:getBulletDebugName(bulletEntity)
    if bulletEntity == nil then
        return "nil"
    end

    return bulletEntity:getName() .. "#" .. tostring(bulletEntity:getId())
end

function BulletManager:resetBulletEntity(bulletEntity)
    if bulletEntity == nil then
        return
    end

    if self:shouldDebugBullet(bulletEntity) then
        Engine.log("Reset bullet: "
            .. self:getBulletDebugName(bulletEntity)
            .. " enabled="
            .. tostring(bulletEntity:isEnabled()))
    end

    if bulletEntity:isEnabled() then
        bulletEntity:sendEvent("ClearOwner")
    end

    bulletEntity:setEnabled(false)

    if self.ownerEntity ~= nil then
        bulletEntity:setParent(self.ownerEntity, false)
    end

    local transform = bulletEntity:getTransform()
    if transform ~= nil then
        transform:setPosition(Vector2F.new(0.0, 0.0))
        transform:setRotation(0.0)
    end
end

function BulletManager:createPool()
    for index = 1, self.poolSize do
        local bulletEntity = spawnPrefab(self.bulletPrefabName, 0.0, 0.0, 0.0)

        if bulletEntity == nil then
            Engine.log("BulletManager failed to spawn pooled bullet " .. tostring(index))
        else
            bulletEntity:setName("PooledBullet" .. tostring(index))
            bulletEntity:setTag("Bullet")
            self:resetBulletEntity(bulletEntity)

            if self:shouldDebugBullet(bulletEntity) then
                Engine.log("Created bullet: "
                    .. self:getBulletDebugName(bulletEntity))
            end

            table.insert(self.bullets, bulletEntity)
        end
    end

    Engine.log("BulletManager created "
        .. tostring(#self.bullets)
        .. "/"
        .. tostring(self.poolSize)
        .. " pooled bullets.")
end

function BulletManager:getAvailableBullet()
    for _, bulletEntity in ipairs(self.bullets) do
        if bulletEntity ~= nil and not bulletEntity:isEnabled() then
            return bulletEntity
        end
    end

    return nil
end

function BulletManager:fire(position, rotation, ownerEntity)
    local bulletEntity = self:getAvailableBullet()
    if bulletEntity == nil then
        Engine.log("BulletManager has no available bullet.")
        self:debugActiveBullets("No available bullet")
        return nil
    end

    bulletEntity:clearParent(false)

    local direction = self:getForwardDirection(rotation)
    local spawnPosition = Vector2F.new(
        position.x + direction.x * self.spawnOffset,
        position.y + direction.y * self.spawnOffset
    )

    local transform = bulletEntity:getTransform()
    if transform ~= nil then
        transform:setPosition(spawnPosition)
        transform:setRotation(rotation)
    end

    if self:shouldDebugBullet(bulletEntity) then
        local ownerName = ownerEntity == nil and "nil" or ownerEntity:getName()
        Engine.log("Firing bullet: "
            .. self:getBulletDebugName(bulletEntity)
            .. " rotation="
            .. tostring(rotation)
            .. " input="
            .. formatPosition(position)
            .. " spawn="
            .. formatPosition(spawnPosition)
            .. " offset="
            .. tostring(self.spawnOffset)
            .. " owner="
            .. tostring(ownerName))
    end

    bulletEntity:setEnabled(true)

    if ownerEntity ~= nil then
        bulletEntity:sendEvent("SetOwner", { owner = ownerEntity })
    else
        bulletEntity:sendEvent("ClearOwner")
    end

    return bulletEntity
end

function BulletManager:debugActiveBullets(context)
    if not self.debugBullets then
        return
    end

    for _, bulletEntity in ipairs(self.bullets) do
        if bulletEntity ~= nil and bulletEntity:isEnabled() then
            local transform = bulletEntity:getTransform()
            local position = transform == nil and nil or transform:getPosition()
            Engine.log(context
                .. ": active "
                .. self:getBulletDebugName(bulletEntity)
                .. " pos="
                .. formatPosition(position)
                .. " inViewport="
                .. tostring(bulletEntity:isInViewport()))
        end
    end
end

function BulletManager:update()
    for _, bulletEntity in ipairs(self.bullets) do
        if bulletEntity ~= nil then
            if bulletEntity:isEnabled() and not bulletEntity:isInViewport() then
                self:resetBulletEntity(bulletEntity)
            end
        end
    end
end

local bulletManager = nil

function fire(position, rotation, ownerEntity)
    if bulletManager == nil then
        Engine.log("BulletManager fire called before initialization.")
        return nil
    end

    return bulletManager:fire(position, rotation, ownerEntity)
end

function resetBullet(bulletEntity)
    if bulletManager == nil then
        Engine.log("BulletManager reset called before initialization.")
        return false
    end

    bulletManager:resetBulletEntity(bulletEntity)
    return true
end

function onEvent(entity, event)
    if event == nil then
        return false
    end

    if event.type == "Fire" then
        local bullet = fire(event.position, event.rotation or 0.0, event.owner)
        event.bullet = bullet
        return bullet ~= nil
    end

    if event.type == "ResetBullet" then
        return resetBullet(event.bullet)
    end

    return false
end

function onStart(entity, script)
    bulletManager = BulletManager:new(entity)

    if script ~= nil then
        bulletManager.bulletPrefabName = script:getPrefab("bulletPrefab", bulletManager.bulletPrefabName)
        bulletManager.poolSize = script:getInt("poolSize", bulletManager.poolSize)
        bulletManager.spawnOffset = script:getFloat("spawnOffset", bulletManager.spawnOffset)
        bulletManager.debugBullets = script:getBool("debugBullets", bulletManager.debugBullets)
        bulletManager.debugAllBullets = script:getBool("debugAllBullets", bulletManager.debugAllBullets)
        bulletManager.debugBulletName = script:getString("debugBulletName", bulletManager.debugBulletName)
    end

    if bulletManager.poolSize < 1 then
        bulletManager.poolSize = 1
    end

    if bulletManager.spawnOffset < 0.0 then
        bulletManager.spawnOffset = 0.0
    end

    bulletManager:createPool()
end

function onUpdate(entity, deltaTime)
    if bulletManager == nil then
        return
    end

    bulletManager:update(deltaTime)
end
