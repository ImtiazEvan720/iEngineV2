#pragma once

#include "components/Component.h"
#include "system/IRenderBackend.h"

#include <string>

enum class VerticalTextAlign {
    top,
    center,
    bottom
};

enum class HorizontalTextAlign {
    left,
    center,
    right
};

class UILabelComponent : public Component {
public:
    UILabelComponent() = default;
    ~UILabelComponent() = default;
    std::unique_ptr<Component> clone() const override;

    void setText(const std::string& value);
    const std::string& getText() const;
    void setFont(const std::string& fontName);
    const std::string& getFontName() const;
    void setFontSize(int size);
    int getFontSize() const;
    void setFontColor(const RenderColor& color);
    const RenderColor& getFontColor() const;
    void setHorizontalTextAlign(HorizontalTextAlign value);
    HorizontalTextAlign getHorizontalTextAlign() const;
    void setVerticalTextAlign(VerticalTextAlign value);
    VerticalTextAlign getVerticalTextAlign() const;

private:
    std::string text = "Label";
    std::string fontName = "Assets/Fonts/default.ttf";
    int fontSize = 18;
    RenderColor fontColor{255, 255, 255, 255};
    HorizontalTextAlign horizontalAlign = HorizontalTextAlign::center;
    VerticalTextAlign verticalAlign = VerticalTextAlign::center;
};
