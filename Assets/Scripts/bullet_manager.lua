ScriptProperties = {
    { name = "bulletPrefab", type = "prefab", default = "Bullet" },
    { name = "poolSize", type = "int", default = 50 },
}

local BulletManager = {}
BulletManager.__index = BulletManager

function BulletManager:new(ownerEntity)
    return setmetatable({
        ownerEntity = ownerEntity,
        bulletPrefabName = "Bullet",
        poolSize = 50,
        bullets = {},
    }, BulletManager)
end

function BulletManager:resetBulletEntity(bulletEntity)
    if bulletEntity == nil then
        return
    end

    bulletEntity:setEnabled(false)

    if self.ownerEntity ~= nil then
        bulletEntity:setParent(self.ownerEntity, false)
    end

    local transform = bulletEntity:getTransform()
    if transform ~= nil then
        transform:setPosition(Vector2F(0.0, 0.0))
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

function BulletManager:fire(position, rotation)
    local bulletEntity = self:getAvailableBullet()
    if bulletEntity == nil then
        Engine.log("BulletManager has no available bullet.")
        return nil
    end

    bulletEntity:clearParent(false)

    local transform = bulletEntity:getTransform()
    if transform ~= nil then
        transform:setPosition(position)
        transform:setRotation(rotation)
    end

    bulletEntity:setEnabled(true)
    return bulletEntity
end

function BulletManager:update()
    for _, bulletEntity in ipairs(self.bullets) do
        if bulletEntity ~= nil and bulletEntity:isEnabled() and not bulletEntity:isInViewport() then
            self:resetBulletEntity(bulletEntity)
        end
    end
end

local bulletManager = nil

function onStart(entity, script)
    bulletManager = BulletManager:new(entity)

    if script ~= nil then
        bulletManager.bulletPrefabName = script:getPrefab("bulletPrefab", bulletManager.bulletPrefabName)
        bulletManager.poolSize = script:getInt("poolSize", bulletManager.poolSize)
    end

    if bulletManager.poolSize < 1 then
        bulletManager.poolSize = 1
    end

    bulletManager:createPool()
end

function onUpdate(entity, deltaTime)
    if bulletManager == nil then
        return
    end

    bulletManager:update(deltaTime)
end
