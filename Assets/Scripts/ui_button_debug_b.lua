local clickCount = 0

local function log(message)
    if Engine ~= nil and Engine.log ~= nil then
        Engine.log(message)
    elseif engineLog ~= nil then
        engineLog(message)
    else
        print(message)
    end
end

function onStart(entity)
    log("[UIButtonDebugB] attached to " .. entity:getName())
end

function onClick(entity)
    clickCount = clickCount + 1
    log("[UIButtonDebugB] onClick from " .. entity:getName()
        .. " count=" .. tostring(clickCount))
end

function onStartGameClick(entity)
    log("[UIButtonDebugB] onStartGameClick from " .. entity:getName())
end

function onQuitClick(entity)
    log("[UIButtonDebugB] onQuitClick from " .. entity:getName())
end
