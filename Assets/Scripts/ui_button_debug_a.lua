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
    log("[UIButtonDebugA] attached to " .. entity:getName())
end

function onClick(entity)
    clickCount = clickCount + 1
    log("[UIButtonDebugA] onClick from " .. entity:getName()
        .. " count=" .. tostring(clickCount))
end

function onDebugAltClick(entity)
    log("[UIButtonDebugA] onDebugAltClick from " .. entity:getName())
end
