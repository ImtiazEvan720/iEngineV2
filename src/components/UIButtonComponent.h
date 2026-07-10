#pragma once
#include "components/Component.h"
#include "system/IRenderBackend.h"
#include <string>

enum class UIButtonState {
    Normal,
    Hovered,
    Pressed,
    Disabled
};

class UIButtonComponent:public Component
{
    public:
    void setAction(const std::string& value);
    const std::string& getAction() const;


    void setFocus(bool value);
    bool isFocused() const;

    void setHovered(bool value);
    bool isHovered() const;

    void setInteractable(bool value);
    bool isInteractable() const;

    bool wasClickedThisFrame() const;
    void setClickedThisFrame(bool value);

    void setPressed(bool value);
    bool isPressed() const;
    
    const RenderColor& getNormalColor() const;
    const RenderColor& getHoverColor() const;
    const RenderColor& getPressedColor() const;
    const RenderColor& getDisabledColor() const;

    void setNormalColor(const RenderColor& value);
    void setHoverColor(const RenderColor& value);
    void setPressedColor(const RenderColor& value);
    void setDisabledColor(const RenderColor& value);

    UIButtonState getState() const;
    RenderColor getCurrentColor() const;
    void resetFrameState();

    std::unique_ptr<Component> clone() const override;

    private:
    bool interactable = true;
    bool focused = false;
    bool hovered = false;
    bool pressed = false;
    bool clickedThisFrame = false;

    std::string action = "";

    RenderColor normalColor{60, 60, 70, 255};
    RenderColor hoverColor{80, 80, 95, 255};
    RenderColor pressedColor{45, 45, 55, 255};
    RenderColor disabledColor{40, 40, 45, 120};

};
