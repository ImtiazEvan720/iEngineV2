#pragma once

#include "components/Component.h"
#include "system/IRenderBackend.h"

#include <cstddef>
#include <string>

class UIEditTextComponent : public Component {
public:
    const std::string& getText() const;
    const std::string& getPlaceholder() const;
    const std::string& getSubmitFunction() const;
    const std::string& getFontName() const;

    const RenderColor& getTextColor() const;
    const RenderColor& getPlaceholderColor() const;
    const RenderColor& getBackgroundColor() const;
    const RenderColor& getFocusedColor() const;
    const RenderColor& getCursorColor() const;

    int getFontSize() const;
    std::size_t getCursorIndex() const;
    int getMaxLength() const;

    bool isFocused() const;
    bool isReadOnly() const;
    bool isPassword() const;

    void setText(const std::string& value);
    void setPlaceholder(const std::string& value);
    void setSubmitFunction(const std::string& value);
    void setFontName(const std::string& value);

    void setTextColor(const RenderColor& value);
    void setPlaceholderColor(const RenderColor& value);
    void setBackgroundColor(const RenderColor& value);
    void setFocusedColor(const RenderColor& value);
    void setCursorColor(const RenderColor& value);

    void setFontSize(int value);
    void setCursorIndex(std::size_t value);
    void setMaxLength(int value);

    void setFocused(bool value);
    void setReadOnly(bool value);
    void setPassword(bool value);

    void insertText(const std::string& value);
    void backspace();
    void clear();

    std::string getDisplayText() const;

    std::unique_ptr<Component> clone() const override;

private:
    std::string text;
    std::string placeholder = "Enter text...";
    std::string submitFunction = "onSubmit";
    std::string fontName = "Assets/Fonts/default.ttf";

    RenderColor textColor{255, 255, 255, 255};
    RenderColor placeholderColor{180, 180, 180, 180};
    RenderColor backgroundColor{35, 35, 42, 255};
    RenderColor focusedColor{55, 70, 95, 255};
    RenderColor cursorColor{255, 255, 255, 255};

    int fontSize = 18;
    std::size_t cursorIndex = 0;
    int maxLength = 64;

    bool focused = false;
    bool readOnly = false;
    bool password = false;
};
