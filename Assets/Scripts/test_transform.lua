local elapsedTime = 0.0
local startPosition = nil

function onStart(entity)
    local transform = entity:getTransform()
    if transform == nil then
        engineLog("onStart: entity has no TransformComponent")
        return
    end

    local position = transform:getPosition()
    startPosition = Vector2F.new(position.x, position.y)

    engineLog("onStart: " .. entity:getName()
        .. " position=(" .. position.x .. ", " .. position.y .. ")")

    engineLog("onStart: testing from name: " .. entity:getName()
             ..", tag: " .. entity:getTag() )
end

function onUpdate(entity, deltaTime)
    local transform = entity:getTransform()
    if transform == nil or startPosition == nil then
        return
    end

    elapsedTime = elapsedTime + deltaTime

    local position = transform:getPosition()
    position.x = startPosition.x + math.sin(elapsedTime * 2.0) * 48.0
    position.y = startPosition.y + math.cos(elapsedTime * 2.0) * 12.0
    transform:setPosition(position)
end
