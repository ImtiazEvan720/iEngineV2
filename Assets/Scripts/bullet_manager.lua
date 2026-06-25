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
        return nil
    end

    bulletEntity:clearParent(false)

    local transform = bulletEntity:getTransform()
    if transform ~= nil then
        transform:setPosition(position)
        transform:setRotation(rotation)
    end

    bulletEntity:setEnabled(true)

    if ownerEntity ~= nil then
        bulletEntity:callScript("setOwner", ownerEntity)
    else
        bulletEntity:callScript("clearOwner")
    end

    return bulletEntity
end

function BulletManager:update()
    for _, bulletEntity in ipairs(self.bullets) do
        if bulletEntity ~= nil then
            if bulletEntity:isEnabled() and not bulletEntity:isInViewport() then
                self:resetBulletEntity(bulletEntity)
            elseif not bulletEntity:isEnabled() then
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

    bulletEntity:callScript("clearOwner")
    bulletManager:resetBulletEntity(bulletEntity)
    return true
end

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
