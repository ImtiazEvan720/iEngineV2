ScriptProperties = {
    { name = "delaySeconds", type = "float", default = 0.0 },
}

local delaySeconds = 0.0
local elapsedSeconds = 0.0
local started = false

local function refreshProperties(script)
    if Props ~= nil then

        if Props.delaySeconds ~= nil then
            delaySeconds = Props.delaySeconds
        end
    end

    if script == nil then
        return
    end

    if Props == nil or Props.delaySeconds == nil then
        delaySeconds = script:getFloat("delaySeconds", delaySeconds)
    end
end

local function getAnimationComponent(entity)
    if entity == nil then
        return nil
    end

    return entity:getAnimation()
end

local function startEffect(entity)
    local animation = getAnimationComponent(entity)
    if animation == nil then
        Engine.log("Effect has no AnimationComponent: " .. entity:getName())
        return
    end

    animation:setEnabled(true)
    animation:setLooping(false)
    animation:reset()
    animation:play()
    started = true
end

function onStart(entity, script)
    refreshProperties(script)

    elapsedSeconds = 0.0
    started = false

    if delaySeconds <= 0.0 then
        startEffect(entity)
    end
end

function onUpdate(entity, deltaTime, script)
    refreshProperties(script)

    if started then
        return
    end

    elapsedSeconds = elapsedSeconds + deltaTime
    if elapsedSeconds >= delaySeconds then
        startEffect(entity)
    end
end

function onAnimationFinished(entity)

    local label = entity:getName()

    if label ~= nil and label ~= "" then
        Engine.log("Effect animation finished: " .. label)
    end
    
    entity:setEnabled(false)
    entity:destroy()
end
