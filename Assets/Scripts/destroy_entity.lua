ScriptProperties = {
    { name = "destroyChildren", type = "bool", default = true },
    { name = "fallbackDestroyDelay", type = "float", default = 0.0 },
}

local destroyChildren = true
local fallbackDestroyDelay = 0.0
local started = false
local fallbackTimer = 0.0

local function refreshProperties(script)
    if Props ~= nil then
        if Props.destroyChildren ~= nil then
            destroyChildren = Props.destroyChildren
        end

        if Props.fallbackDestroyDelay ~= nil then
            fallbackDestroyDelay = Props.fallbackDestroyDelay
        end
    end

    if script == nil then
        return
    end

    if Props == nil or Props.destroyChildren == nil then
        destroyChildren = script:getBool("destroyChildren", destroyChildren)
    end

    if Props == nil or Props.fallbackDestroyDelay == nil then
        fallbackDestroyDelay = script:getFloat("fallbackDestroyDelay", fallbackDestroyDelay)
    end
end

local function startDestroyAnimation(entity)
    local animation = entity:getAnimation()
    if animation == nil then
        Engine.log("DestroyEntity has no AnimationComponent: " .. entity:getName())
        fallbackTimer = fallbackDestroyDelay
        started = true
        return
    end

    animation:setLooping(false)
    animation:reset()
    animation:play()

    started = true
end

function onStart(entity, script)
    refreshProperties(script)

    local animation = entity:getAnimation()
    if animation ~= nil then
        animation:setLooping(false)
        animation:reset()
        animation:pause()
    end
end

function onUpdate(entity, deltaTime, script)
    refreshProperties(script)

    if not started then
        startDestroyAnimation(entity)
        return
    end

    local animation = entity:getAnimation()
    if animation ~= nil then
        if animation:isFinished() then
            entity:destroy(destroyChildren)
        end

        return
    end

    fallbackTimer = fallbackTimer - deltaTime
    if fallbackTimer <= 0.0 then
        entity:destroy(destroyChildren)
    end
end
