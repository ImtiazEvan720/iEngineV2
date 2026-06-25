ScriptProperties = {
    { name = "speed", type = "float", default = 70.0 },
    { name = "fireCooldown", type = "float", default = 10 },
    { name = "aiWaitTime", type = "float", default = 4.0 },
    { name = "alignmentTolerance", type = "float", default = 12.0 },
    { name = "viewportMargin", type = "float", default = 16.0 },
    { name = "obstacleTag", type = "string", default = "Obstacle" },
    { name = "obstacleRayDistance", type = "float", default = 48.0 },
    { name = "moveTimeMin", type = "float", default = 0.75 },
    { name = "moveTimeMax", type = "float", default = 2.0 },
    { name = "target", type = "entity", default = "" },
    { name = "bulletManager", type = "entity", default = "" },
    { name = "turret", type = "entity", default = "" },
}

local directions = {
    { x = 0.0, y = -1.0, rotation = 0.0 },
    { x = 1.0, y = 0.0, rotation = 90.0 },
    { x = 0.0, y = 1.0, rotation = 180.0 },
    { x = -1.0, y = 0.0, rotation = -90.0 },
}

local EnemyTank = {}
EnemyTank.__index = EnemyTank

local function randomRange(minValue, maxValue)
    return minValue + (maxValue - minValue) * math.random()
end

function EnemyTank:new(entity)
    local enemyTank = setmetatable({
        entity = entity,
        speed = 70.0,
        fireCooldown = 1.5,
        fireTimer = 0.0,
        aiWaitTime = 4.0,
        aiWaitTimer = 0.0,
        alignmentTolerance = 12.0,
        viewportMargin = 16.0,
        obstacleTag = "Obstacle",
        obstacleRayDistance = 48.0,
        moveTimeMin = 0.75,
        moveTimeMax = 2.0,
        moveTimer = 0.0,
        direction = directions[1],
        target = nil,
        bulletManager = nil,
        turret = nil,
    }, EnemyTank)

    enemyTank:resetAimWaitTimer()
    return enemyTank
end

function EnemyTank:refreshProperties()
    if Props ~= nil then
        if Props.speed ~= nil then
            self.speed = Props.speed
        end

        if Props.fireCooldown ~= nil then
            self.fireCooldown = Props.fireCooldown
        end

        if Props.alignmentTolerance ~= nil then
            self.alignmentTolerance = Props.alignmentTolerance
        end

        if Props.viewportMargin ~= nil then
            self.viewportMargin = Props.viewportMargin
        end

        if Props.obstacleTag ~= nil then
            self.obstacleTag = Props.obstacleTag
        end

        if Props.obstacleRayDistance ~= nil then
            self.obstacleRayDistance = Props.obstacleRayDistance
        end

        if Props.moveTimeMin ~= nil then
            self.moveTimeMin = Props.moveTimeMin
        end

        if Props.moveTimeMax ~= nil then
            self.moveTimeMax = Props.moveTimeMax
        end

        if Props.aiWaitTime ~= nil then
            self.aiWaitTime = Props.aiWaitTime
        end
    end

    if Refs ~= nil then
        if Refs.target ~= nil then
            self.target = Refs.target
        end

        if Refs.bulletManager ~= nil then
            self.bulletManager = Refs.bulletManager
        end

        if Refs.turret ~= nil then
            self.turret = Refs.turret
        end
    end

    if self.target == nil then
        self.target = Engine.findEntityByName("PlayerTank")
    end

    if self.bulletManager == nil then
        self.bulletManager = Engine.findEntityByName("BulletManager")
    end
end

function EnemyTank:chooseNewDirection()
    self.direction = directions[math.random(1, #directions)]
    self.moveTimer = randomRange(self.moveTimeMin, self.moveTimeMax)
end

function EnemyTank:resetAimWaitTimer()
    self.aiWaitTimer = self.aiWaitTime
end

function EnemyTank:isObstacleAhead(transform)
    local worldPosition = transform:getWorldPosition()
    local rayEnd = Vector2F.new(
        worldPosition.x + self.direction.x * self.obstacleRayDistance,
        worldPosition.y + self.direction.y * self.obstacleRayDistance
    )

    local hit = Engine.raycast(worldPosition, rayEnd)
    if hit == nil or not hit.hit then
        return false
    end

    local hitTag = string.lower(hit.tag or "")
    local obstacleTag = string.lower(self.obstacleTag or "Obstacle")
    return hitTag == obstacleTag
end

function EnemyTank:getFireTransform(transform)
    if self.turret ~= nil and self.turret:getTransform() ~= nil then
        return self.turret:getTransform()
    end

    return transform
end

function EnemyTank:getTargetAimDirection(transform)
    if self.target == nil or self.target:getTransform() == nil then
        return nil
    end

    local myPosition = transform:getWorldPosition()
    local targetPosition = self.target:getTransform():getWorldPosition()
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

function EnemyTank:fire(transform)
    if self.bulletManager == nil then
        return
    end

    local fireTransform = self:getFireTransform(transform)
    local position = fireTransform:getWorldPosition()
    local rotation = fireTransform:getWorldRotation()

    if self.bulletManager:callScript("fire", position, rotation) then
        self.fireTimer = self.fireCooldown
    end
end

function EnemyTank:moveForward(transform, deltaTime)
    local distance = self.speed * deltaTime
    local worldPosition = transform:getWorldPosition()

    if self:isObstacleAhead(transform) then
        self:chooseNewDirection()
        self.moveTimer = 0.0
        return
    end

    local nextWorldPosition = Vector2F.new(
        worldPosition.x + self.direction.x * distance,
        worldPosition.y + self.direction.y * distance
    )

    if not Engine.isWorldPointInViewport(nextWorldPosition, self.viewportMargin) then
        self:chooseNewDirection()
        self.moveTimer = 0.0
        return
    end

    local position = transform:getPosition()
    position.x = position.x + self.direction.x * distance
    position.y = position.y + self.direction.y * distance
    transform:setPosition(position)
    transform:setRotation(self.direction.rotation)
end

function EnemyTank:update(deltaTime)
    local transform = self.entity:getTransform()
    if transform == nil then
        return
    end

    self.fireTimer = self.fireTimer - deltaTime
    self.moveTimer = self.moveTimer - deltaTime

    local aimDirection = self:getTargetAimDirection(transform)
    if aimDirection ~= nil then
        self.direction = aimDirection
        transform:setRotation(self.direction.rotation)
        self.aiWaitTimer = self.aiWaitTimer - deltaTime

        if self.fireTimer <= 0.0 and self.aiWaitTimer <= 0.0 then
            self:fire(transform)
            self:resetAimWaitTimer()
        end
    else
        if self.moveTimer <= 0.0 then
            self:chooseNewDirection()
        end

        self:moveForward(transform, deltaTime)
    end
end

local enemyTank = nil

function onStart(entity, script)
    math.randomseed(entity:getId() * 1103515245)
    enemyTank = EnemyTank:new(entity)
    enemyTank:refreshProperties()
    enemyTank:resetAimWaitTimer()
    enemyTank:chooseNewDirection()
    Engine.log("EnemyTank started: " .. entity:getName())
end

function onUpdate(entity, deltaTime, script)
    if enemyTank == nil then
        enemyTank = EnemyTank:new(entity)
        enemyTank:refreshProperties()
    end

    enemyTank:update(deltaTime)
end
