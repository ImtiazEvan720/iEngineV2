function onStart(entity)
    Engine.log("[UIEditTextSubmit] attached to " .. entity:getName())
end

function onSubmit(entity)
    local editText = entity:getEditText()
    if editText == nil then
        Engine.log("[UIEditTextSubmit] entity has no UIEditTextComponent: " .. entity:getName())
        return
    end

    Engine.log("[UIEditTextSubmit] submitted text: " .. editText:getText())
end
