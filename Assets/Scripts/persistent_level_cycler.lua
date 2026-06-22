local levelIndex = 1
local activeLevels = {}

local function isStartupLevel(levelName)
    local normalizedName = string.lower(levelName or "")
    return normalizedName == "loader"
        or normalizedName == "loader.ilevel"
        or string.match(normalizedName, "/loader%.ilevel$") ~= nil
end

local function cacheActiveLevels()
    activeLevels = {}

    for _, levelName in ipairs(Engine.getActiveLevels()) do
        if not isStartupLevel(levelName) then
            table.insert(activeLevels, levelName)
        end
    end

    if levelIndex > #activeLevels then
        levelIndex = 1
    end
end

local function cycleActiveLevel()
    local activeLevelCount = #activeLevels
    if activeLevelCount == 0 then
        Engine.log("No active levels found.")
        return
    end

    local levelName = activeLevels[levelIndex]
    if levelName == nil or levelName == "" then
        Engine.log("Invalid active level index: " .. tostring(levelIndex))
        levelIndex = 1
        return
    end

    Engine.log("Loading active level: " .. levelName)
    Engine.loadLevel(levelName)

    levelIndex = levelIndex + 1
    if levelIndex > activeLevelCount then
        levelIndex = 1
    end
end

function onStart(entity)
    entity:setPersistent(true)
    cacheActiveLevels()
    Engine.log("Persistent level cycler started: " .. entity:getName())
end

function onUpdate(entity, deltaTime)
    if Input.wasActionPressed("Fire2") then
        cycleActiveLevel()
    end
end
