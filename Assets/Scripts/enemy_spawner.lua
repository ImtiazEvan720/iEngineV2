ScriptProperties = {
    {
        name = "SpawnRate",
        type = "float",
        default = 1.0
    },

    {
        name = "MaxEnemies",
        type = "int",
        default = 10
    },

    {
        name = "EnemyPrefabs",
        type = "map",
        valueType = "prefab",
        default = {}
    },

    {
        name = "SpawnArea",
        type = "array",
        elementType = "entity",
        default = {}
    }
}

local spawnCooldown = 1.0
local maxEnemies = 10
local spawnTimer = 0.0
local enemyPrefabNames = {}
local spawnPointEntities = {}
local spawnedEnemies = {}

local function refreshProperties()
    enemyPrefabNames = {}
    spawnPointEntities = {}

    if Props ~= nil and Props.SpawnRate ~= nil then
        spawnCooldown = Props.SpawnRate
    end

    if Props ~= nil and Props.MaxEnemies ~= nil then
        maxEnemies = Props.MaxEnemies
    end

    if spawnCooldown < 0.01 then
        spawnCooldown = 0.01
    end

    if maxEnemies < 0 then
        maxEnemies = 0
    end

    if Props ~= nil and Props.EnemyPrefabs ~= nil then
        for _, prefabName in pairs(Props.EnemyPrefabs) do
            if prefabName ~= nil and prefabName ~= "" then
                table.insert(enemyPrefabNames, prefabName)
            end
        end
    end

    if Props ~= nil and Props.SpawnArea ~= nil then
        for _, spawnEntity in ipairs(Props.SpawnArea) do
            if spawnEntity ~= nil then
                table.insert(spawnPointEntities, spawnEntity)
            end
        end
    end
end

local function removeDestroyedOrDisabledEnemies()
    local aliveEnemies = {}

    for _, enemyEntity in ipairs(spawnedEnemies) do
        if enemyEntity ~= nil and enemyEntity:isEnabled() then
            table.insert(aliveEnemies, enemyEntity)
        end
    end

    spawnedEnemies = aliveEnemies
end

local function getRandomPrefabName()
    if #enemyPrefabNames == 0 then
        return nil
    end

    return enemyPrefabNames[math.random(1, #enemyPrefabNames)]
end

local function getRandomSpawnPoint()
    if #spawnPointEntities == 0 then
        return nil
    end

    return spawnPointEntities[math.random(1, #spawnPointEntities)]
end

local function spawnEnemy()
    removeDestroyedOrDisabledEnemies()

    if #spawnedEnemies >= maxEnemies then
        return
    end

    local prefabName = getRandomPrefabName()
    if prefabName == nil then
        Engine.log("EnemySpawner has no enemy prefabs.")
        return
    end

    local spawnPoint = getRandomSpawnPoint()
    if spawnPoint == nil then
        Engine.log("EnemySpawner has no spawn points.")
        return
    end

    local spawnTransform = spawnPoint:getTransform()
    if spawnTransform == nil then
        Engine.log("EnemySpawner spawn point has no TransformComponent: " .. spawnPoint:getName())
        return
    end

    local position = spawnTransform:getWorldPosition()
    local rotation = spawnTransform:getWorldRotation()
    local enemy = spawnPrefab(prefabName, position, rotation)

    if enemy == nil then
        Engine.log("EnemySpawner failed to spawn prefab: " .. tostring(prefabName))
        return
    end

    enemy:setTag("Enemy")
    table.insert(spawnedEnemies, enemy)
    Engine.log("EnemySpawner spawned " .. prefabName .. " at " .. spawnPoint:getName())
end

function onStart(entity, script)
    refreshProperties()
    spawnTimer = spawnCooldown

    Engine.log("EnemySpawner started with "
        .. tostring(#enemyPrefabNames)
        .. " prefab(s), "
        .. tostring(#spawnPointEntities)
        .. " spawn point(s), cooldown="
        .. tostring(spawnCooldown)
        .. ", maxEnemies="
        .. tostring(maxEnemies))
end

function onUpdate(entity, deltaTime, script)
    refreshProperties()
    removeDestroyedOrDisabledEnemies()

    spawnTimer = spawnTimer - deltaTime
    if spawnTimer > 0.0 then
        return
    end

    spawnTimer = spawnCooldown
    spawnEnemy()
end
