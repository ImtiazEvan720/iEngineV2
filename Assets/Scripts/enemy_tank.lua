ScriptProperties = {{
    name = "speed",
    type = "float",
    default = 70.0
}, {
    name = "life",
    type = "float",
    default = 3.0
}, {
    name = "fireCooldown",
    type = "float",
    default = 10
}, {
    name = "aiWaitTime",
    type = "float",
    default = 4.0
}, {
    name = "alignmentTolerance",
    type = "float",
    default = 12.0
}, {
    name = "viewportMargin",
    type = "float",
    default = 16.0
}, {
    name = "obstacleTag",
    type = "string",
    default = "Obstacle"
}, {
    name = "obstacleRayDistance",
    type = "float",
    default = 48.0
}, {
    name = "raycastStartOffset",
    type = "float",
    default = 18.0
}, {
    name = "moveTimeMin",
    type = "float",
    default = 0.75
}, {
    name = "moveTimeMax",
    type = "float",
    default = 2.0
}, {
    name = "target",
    type = "entity",
    default = ""
}, {
    name = "bulletManager",
    type = "entity",
    default = ""
}, {
    name = "turret",
    type = "entity",
    default = ""
}, {
    name = "destroyAnimationEntity",
    type = "entity",
    default = ""
}, {
    name = "destroyDelay",
    type = "float",
    default = 0.6
}}

local directions = {{
    x = 0.0,
    y = -1.0,
    rotation = 0.0
}, {
    x = 1.0,
    y = 0.0,
    rotation = 90.0
}, {
    x = 0.0,
    y = 1.0,
    rotation = 180.0
}, {
    x = -1.0,
    y = 0.0,
    rotation = -90.0
}}

local EnemyTank = {}
EnemyTank.__index = EnemyTank

local function isFiniteNumber(value)
    return type(value) == "number" and value == value and value ~= math.huge and value ~= -math.huge
end

local function numberOr(value, fallback)
    if isFiniteNumber(value) then
        return value
    end

    return fallback
end

local function clamp(value, minValue, maxValue)
    value = numberOr(value, minValue)

    if value < minValue then
        return minValue
    end

    if value > maxValue then
        return maxValue
    end

    return value
end

local function randomRange(minValue, maxValue)
    minValue = numberOr(minValue, 0.0)
    maxValue = numberOr(maxValue, minValue)

    if maxValue < minValue then
        minValue, maxValue = maxValue, minValue
    end

    return minValue + (maxValue - minValue) * math.random()
end

local function safeEntityCall(entity, methodName)
    if entity == nil then
        return nil
    end

    local ok, result = pcall(function()
        return entity[methodName](entity)
    end)

    if not ok then
        return nil
    end

    return result
end

local function isEntityUsable(entity)
    if entity == nil then
        return false
    end

    local destroyed = safeEntityCall(entity, "isDestroyed")
    if destroyed == nil or destroyed then
        return false
    end

    local enabled = safeEntityCall(entity, "isEnabled")
    return enabled == true
end

local function isEntityReferenceValid(entity)
    if entity == nil then
        return false
    end

    local destroyed = safeEntityCall(entity, "isDestroyed")
    return destroyed == false
end

local function findUsableEntityByName(names)
    for _, name in ipairs(names) do
        local entity = Engine.findEntityByName(name)
        if isEntityUsable(entity) then
            return entity
        end
    end

    return nil
end

local function findUsableEntityByTag(tags)
    for _, tag in ipairs(tags) do
        local entity = Engine.findEntityByTag(tag)
        if isEntityUsable(entity) then
            return entity
        end
    end

    return nil
end

local blockingTags = {
    obstacle = true,
    tank = true,
    player = true,
    enemy = true
}

function EnemyTank:new(entity)
    local enemyTank = setmetatable({
        entity = entity,
        speed = 70.0,
        life = 3.0,
        lifeInitialized = false,
        destroyed = false,
        fireCooldown = 1.5,
        fireTimer = 0.0,
        aiWaitTime = 4.0,
        aiWaitTimer = 0.0,
        alignmentTolerance = 12.0,
        viewportMargin = 16.0,
        obstacleTag = "Obstacle",
        obstacleRayDistance = 48.0,
        raycastStartOffset = 18.0,
        moveTimeMin = 0.75,
        moveTimeMax = 2.0,
        moveTimer = 0.0,
        direction = directions[1],
        target = nil,
        bulletManager = nil,
        turret = nil,
        destroyAnimationEntity = nil,
        destroyDelay = 0.6,
        destroyTimer = 0.0,
        pendingDestroy = false,
        missingDependencyLogTimer = 0.0,
        animationMoving = false
    }, EnemyTank)

    enemyTank:resetAimWaitTimer()
    return enemyTank
end

function EnemyTank:refreshProperties()
    if Props ~= nil then
        if Props.speed ~= nil then
            self.speed = clamp(Props.speed, 0.0, 1000.0)
        end

        if not self.lifeInitialized and Props.life ~= nil then
            self.life = clamp(Props.life, 0.0, 1000000.0)
            self.lifeInitialized = true
        end

        if Props.fireCooldown ~= nil then
            self.fireCooldown = clamp(Props.fireCooldown, 0.1, 120.0)
        end

        if Props.alignmentTolerance ~= nil then
            self.alignmentTolerance = clamp(Props.alignmentTolerance, 0.0, 256.0)
        end

        if Props.viewportMargin ~= nil then
            self.viewportMargin = clamp(Props.viewportMargin, 0.0, 1024.0)
        end

        if Props.obstacleTag ~= nil then
            self.obstacleTag = Props.obstacleTag
        end

        if Props.obstacleRayDistance ~= nil then
            self.obstacleRayDistance = clamp(Props.obstacleRayDistance, 1.0, 1024.0)
        end

        if Props.raycastStartOffset ~= nil then
            self.raycastStartOffset = clamp(Props.raycastStartOffset, 0.0, 256.0)
        end

        if Props.moveTimeMin ~= nil then
            self.moveTimeMin = clamp(Props.moveTimeMin, 0.05, 120.0)
        end

        if Props.moveTimeMax ~= nil then
            self.moveTimeMax = clamp(Props.moveTimeMax, 0.05, 120.0)
        end

        if Props.aiWaitTime ~= nil then
            self.aiWaitTime = clamp(Props.aiWaitTime, 0.0, 120.0)
        end

        if Props.destroyDelay ~= nil then
            self.destroyDelay = clamp(Props.destroyDelay, 0.0, 30.0)
        end
    end

    if self.moveTimeMax < self.moveTimeMin then
        self.moveTimeMin, self.moveTimeMax = self.moveTimeMax, self.moveTimeMin
    end

    self.target = nil
    self.bulletManager = nil
    self.turret = nil
    self.destroyAnimationEntity = nil

    if Refs ~= nil then
        if isEntityUsable(Refs.target) then
            self.target = Refs.target
        end

        if isEntityUsable(Refs.bulletManager) then
            self.bulletManager = Refs.bulletManager
        end

        if isEntityUsable(Refs.turret) then
            self.turret = Refs.turret
        end

        if isEntityReferenceValid(Refs.destroyAnimationEntity) then
            self.destroyAnimationEntity = Refs.destroyAnimationEntity
        end
    end

    if self.target == nil then
        self.target = findUsableEntityByName({"PlayerTank", "Player_Tank", "Player1", "Player"})
    end

    if self.target == nil then
        self.target = findUsableEntityByTag({"Player", "PlayerTank"})
    end

    if self.bulletManager == nil then
        self.bulletManager = findUsableEntityByName({"BulletManager", "bulletManager"})
    end
end

function EnemyTank:reportMissingDependencies(deltaTime)
    self.missingDependencyLogTimer = self.missingDependencyLogTimer - deltaTime
    if self.missingDependencyLogTimer > 0.0 then
        return
    end

    self.missingDependencyLogTimer = 2.0

    if self.target == nil then
        Engine.log("EnemyTank fallback: no target found for " .. self.entity:getName())
    end

    if self.bulletManager == nil then
        Engine.log("EnemyTank fallback: no BulletManager found for " .. self.entity:getName())
    end
end

function EnemyTank:takeDamage(damage)
    if self.destroyed or self.pendingDestroy then
        return
    end

    local damageAmount = tonumber(damage) or 0.0
    if damageAmount <= 0.0 then
        return
    end

    self.life = self.life - damageAmount
    Engine.log("EnemyTank damaged: " .. self.entity:getName() .. " damage=" .. tostring(damageAmount) .. " life=" ..
                   tostring(self.life))

    if self.life > 0.0 then
        return
    end

    self.pendingDestroy = true
    self.destroyTimer = self.destroyDelay
    self:setMovingAnimation(false)
    self:playDestroyAnimation()
    Engine.log("EnemyTank destruction started: " .. self.entity:getName() .. " delay=" .. tostring(self.destroyDelay))
end

function EnemyTank:getAnimation()
    if self.entity == nil then
        return nil
    end

    local ok, animation = pcall(function()
        return self.entity:getAnimation()
    end)

    if ok then
        return animation
    end

    return nil
end

function EnemyTank:setMovingAnimation(moving)
    if self.animationMoving == moving then
        return
    end

    self.animationMoving = moving

    local animation = self:getAnimation()
    if animation == nil then
        return
    end

    if moving then
        animation:play()
    else
        animation:pause()
    end
end

function EnemyTank:resetAnimation()
    self.animationMoving = false

    local animation = self:getAnimation()
    if animation == nil then
        return
    end

    animation:reset()
    animation:pause()
end

function EnemyTank:playDestroyAnimation()
    if not isEntityReferenceValid(self.destroyAnimationEntity) then
        return
    end

    local tankAnimation = self.entity:getAnimation()
    if tankAnimation ~= nil then
        tankAnimation:pause()
        tankAnimation:setEnabled(false)
    end

    self.destroyAnimationEntity:setEnabled(true)

end

function EnemyTank:updatePendingDestroy(deltaTime)
    self:setMovingAnimation(false)
    self.destroyTimer = self.destroyTimer - deltaTime

    if self.destroyTimer > 0.0 then
        return
    end

    self.destroyed = true
    Engine.log("EnemyTank destroyed: " .. self.entity:getName())
    self.entity:destroy(true)
end

function EnemyTank:chooseNewDirection()
    self.direction = directions[math.random(1, #directions)]
    self.moveTimer = randomRange(self.moveTimeMin, self.moveTimeMax)
end

function EnemyTank:chooseUnblockedDirection(transform)
    if transform == nil then
        self:chooseNewDirection()
        return true
    end

    local startIndex = math.random(1, #directions)
    local probeDistance = math.max(self.speed / 30.0, 1.0)

    for offset = 0, #directions - 1 do
        local directionIndex = ((startIndex + offset - 2) % #directions) + 1
        local direction = directions[directionIndex]

        if self:canMoveInDirection(transform, direction, probeDistance) then
            self.direction = direction
            self.moveTimer = randomRange(self.moveTimeMin, self.moveTimeMax)
            return true
        end
    end

    return false
end

function EnemyTank:resetAimWaitTimer()
    self.aiWaitTimer = self.aiWaitTime
end

function EnemyTank:isBlockingHit(hit)
    if hit == nil or not hit.hit then
        return false
    end

    if hit.entity ~= nil and hit.entity:getId() == self.entity:getId() then
        return false
    end

    local hitTag = string.lower(hit.tag or "")
    local obstacleTag = string.lower(self.obstacleTag or "Obstacle")

    return hitTag == obstacleTag or blockingTags[hitTag] == true
end

function EnemyTank:raycastInDirection(transform, direction, distance)
    local worldPosition = transform:getWorldPosition()
    local rayStart = Vector2F.new(worldPosition.x + direction.x * self.raycastStartOffset,
        worldPosition.y + direction.y * self.raycastStartOffset)
    local rayEnd = Vector2F.new(rayStart.x + direction.x * distance, rayStart.y + direction.y * distance)

    local ok, hit = pcall(function()
        return Engine.raycast(rayStart, rayEnd)
    end)

    if not ok then
        Engine.log("EnemyTank raycast fallback: " .. tostring(hit))
        return nil
    end

    return hit
end

function EnemyTank:isObstacleAhead(transform, direction, distance)
    direction = direction or self.direction
    distance = distance or self.obstacleRayDistance

    local hit = self:raycastInDirection(transform, direction, distance)
    return self:isBlockingHit(hit)
end

function EnemyTank:canMoveInDirection(transform, direction, moveDistance)
    local probeDistance = self.obstacleRayDistance + math.max(moveDistance, 1.0)
    if self:isObstacleAhead(transform, direction, probeDistance) then
        return false
    end

    local worldPosition = transform:getWorldPosition()
    local nextWorldPosition = Vector2F.new(worldPosition.x + direction.x * moveDistance,
        worldPosition.y + direction.y * moveDistance)

    return Engine.isWorldPointInViewport(nextWorldPosition, self.viewportMargin)
end

function EnemyTank:getFireTransform(transform)
    if isEntityUsable(self.turret) then
        local ok, turretTransform = pcall(function()
            return self.turret:getTransform()
        end)

        if ok and turretTransform ~= nil then
            return turretTransform
        end
    end

    return transform
end

function EnemyTank:getTargetAimDirection(transform)
    if not isEntityUsable(self.target) then
        return nil
    end

    local ok, targetTransform = pcall(function()
        return self.target:getTransform()
    end)

    if not ok or targetTransform == nil then
        return nil
    end

    local myPosition = transform:getWorldPosition()
    local targetPosition = targetTransform:getWorldPosition()
    local deltaX = targetPosition.x - myPosition.x
    local deltaY = targetPosition.y - myPosition.y

    if math.abs(deltaX) <= self.alignmentTolerance then
        if deltaY < 0.0 then
            return directions[1]
        end

        return directions[3]
    end

    if math.abs(deltaY) <= self.alignmentTolerance then
        if deltaX > 0.0 then
            return directions[2]
        end

        return directions[4]
    end

    return nil
end

function EnemyTank:hasClearShotToTarget(transform, direction)
    if not isEntityUsable(self.target) then
        return false
    end

    local ok, targetTransform = pcall(function()
        return self.target:getTransform()
    end)

    if not ok or targetTransform == nil then
        return false
    end

    local myPosition = transform:getWorldPosition()
    local targetPosition = targetTransform:getWorldPosition()
    local deltaX = targetPosition.x - myPosition.x
    local deltaY = targetPosition.y - myPosition.y
    local distance = math.max(math.abs(deltaX), math.abs(deltaY)) - self.raycastStartOffset

    if distance <= 0.0 then
        return true
    end

    local hit = self:raycastInDirection(transform, direction, distance)
    if hit == nil or not hit.hit then
        return true
    end

    if hit.entity ~= nil then
        if hit.entity:getId() == self.entity:getId() then
            return true
        end

        if hit.entity:getId() == self.target:getId() then
            return true
        end
    end

    return not self:isBlockingHit(hit)
end

function EnemyTank:fire(transform)
    if not isEntityUsable(self.bulletManager) then
        return false
    end

    local fireTransform = self:getFireTransform(transform)
    local position = fireTransform:getWorldPosition()
    local rotation = fireTransform:getWorldRotation()

    local ok, fired = pcall(function()
        return self.bulletManager:sendEvent("Fire", {
            position = position,
            rotation = rotation,
            owner = self.entity
        })
    end)

    if ok and fired then
        self.fireTimer = self.fireCooldown
        return true
    end

    if not ok then
        Engine.log("EnemyTank fire fallback: " .. tostring(fired))
    end

    self.fireTimer = math.max(0.25, self.fireCooldown * 0.25)
    return false
end

function EnemyTank:moveForward(transform, deltaTime)
    local distance = self.speed * deltaTime
    local worldPosition = transform:getWorldPosition()

    if not self:canMoveInDirection(transform, self.direction, distance) then
        self:chooseUnblockedDirection(transform)
        self.moveTimer = 0.1
        return false
    end

    local nextWorldPosition = Vector2F.new(worldPosition.x + self.direction.x * distance,
        worldPosition.y + self.direction.y * distance)

    if not Engine.isWorldPointInViewport(nextWorldPosition, self.viewportMargin) then
        self:chooseUnblockedDirection(transform)
        self.moveTimer = 0.1
        return false
    end

    local position = transform:getPosition()
    position.x = position.x + self.direction.x * distance
    position.y = position.y + self.direction.y * distance
    transform:setPosition(position)
    transform:setRotation(self.direction.rotation)
    return true
end

function EnemyTank:update(deltaTime)
    if self.destroyed then
        self:setMovingAnimation(false)
        return
    end

    deltaTime = clamp(deltaTime, 0.0, 0.25)

    if self.pendingDestroy then
        self:updatePendingDestroy(deltaTime)
        return
    end

    local transform = self.entity:getTransform()
    if transform == nil then
        self:setMovingAnimation(false)
        self:reportMissingDependencies(deltaTime)
        return
    end

    self:reportMissingDependencies(deltaTime)

    self.fireTimer = self.fireTimer - deltaTime
    self.moveTimer = self.moveTimer - deltaTime

    local aimDirection = self:getTargetAimDirection(transform)
    if aimDirection ~= nil and isEntityUsable(self.bulletManager) then
        if self:hasClearShotToTarget(transform, aimDirection) then
            self.direction = aimDirection
            transform:setRotation(self.direction.rotation)
            self.aiWaitTimer = self.aiWaitTimer - deltaTime

            if self.fireTimer <= 0.0 and self.aiWaitTimer <= 0.0 then
                if not self:fire(transform) then
                    self:chooseUnblockedDirection(transform)
                end
                self:resetAimWaitTimer()
            end
            self:setMovingAnimation(false)
            return
        end
    end

    if self.moveTimer <= 0.0 then
        self:chooseUnblockedDirection(transform)
    end

    local moved = self:moveForward(transform, deltaTime)
    self:setMovingAnimation(moved == true)
end

local enemyTanks = {}
local scriptEntity = nil

local function getEnemyTank(entity)
    if entity == nil then
        return nil
    end

    local entityId = entity:getId()
    local enemyTank = enemyTanks[entityId]

    if enemyTank == nil or enemyTank.entity ~= entity then
        enemyTank = EnemyTank:new(entity)
        enemyTanks[entityId] = enemyTank
    end

    return enemyTank
end

function getLife(entity)
    local enemyTank = getEnemyTank(entity)
    if enemyTank == nil then
        return 0.0
    end

    return enemyTank.life
end

function onEvent(entity, event)
    if event == nil then
        return false
    end

    local enemyTank = getEnemyTank(entity)
    if enemyTank == nil then
        return false
    end

    if event.type == "Damage" then
        enemyTank:takeDamage(event.damage or 0.0)
        event.life = enemyTank.life
        event.destroyed = enemyTank.destroyed or enemyTank.pendingDestroy
        return true
    end

    if event.type == "GetLife" then
        event.life = enemyTank.life
        event.destroyed = enemyTank.destroyed or enemyTank.pendingDestroy
        return true
    end

    return false
end

function onStart(entity, script)
    scriptEntity = entity
    math.randomseed(entity:getId() * 1103515245)
    local enemyTank = getEnemyTank(entity)
    if enemyTank == nil then
        return
    end

    enemyTank:refreshProperties()
    enemyTank:resetAimWaitTimer()
    enemyTank:resetAnimation()
    if isEntityReferenceValid(enemyTank.destroyAnimationEntity) then
        enemyTank.destroyAnimationEntity:setEnabled(false)
    end
    enemyTank:chooseNewDirection()
    Engine.log("EnemyTank started: " .. entity:getName())
end

function onUpdate(entity, deltaTime, script)
    scriptEntity = entity

    local enemyTank = getEnemyTank(entity)
    if enemyTank == nil then
        return
    end

    local ok, errorMessage = pcall(function()
        enemyTank:refreshProperties()
        enemyTank:update(deltaTime)
    end)

    if not ok then
        Engine.log("EnemyTank update fallback: " .. tostring(errorMessage))
        enemyTank:chooseNewDirection()
    end

    if enemyTank.destroyed then
        enemyTanks[entity:getId()] = nil
    end
end

function takeDamage(targetEntityOrDamage, damage)
    local targetEntity = targetEntityOrDamage
    local damageAmount = damage

    if type(targetEntityOrDamage) == "number" then
        targetEntity = scriptEntity
        damageAmount = targetEntityOrDamage
    end

    local enemyTank = getEnemyTank(targetEntity)
    if enemyTank == nil then
        return false
    end

    enemyTank:takeDamage(damageAmount)

    if enemyTank.destroyed and targetEntity ~= nil then
        enemyTanks[targetEntity:getId()] = nil
    end

    return true
end
