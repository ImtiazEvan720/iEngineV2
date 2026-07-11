#pragma once

#include <string>
#include <unordered_map>
#include <vector>

enum class RawKey {
    Unknown,
    W,
    A,
    S,
    D,
    Space,
    Escape,
    Backspace,
    Enter
};

enum class RawMouseButton {
    Unknown,
    Left,
    Right,
    Middle
};

struct RawTouch {
    int id = 0;
    float x = 0.0f;
    float y = 0.0f;
    bool down = false;
};

class RawInputSystem {
public:
    static RawInputSystem& getInstance();

    RawInputSystem(const RawInputSystem& other) = delete;
    RawInputSystem& operator=(const RawInputSystem& other) = delete;
    RawInputSystem(RawInputSystem&& other) = delete;
    RawInputSystem& operator=(RawInputSystem&& other) = delete;

    void beginFrame();
    void endFrame();

    void setKeyDown(RawKey key);
    void setKeyUp(RawKey key);
    void addTextInput(const std::string& text);

    void setMouseButtonDown(RawMouseButton button, int x, int y);
    void setMouseButtonUp(RawMouseButton button, int x, int y);
    void setMousePosition(int x, int y);

    void setTouchDown(int touchId, float x, float y);
    void setTouchMove(int touchId, float x, float y);
    void setTouchUp(int touchId, float x, float y);

    bool isKeyDown(RawKey key) const;
    bool wasKeyPressed(RawKey key) const;
    bool wasKeyReleased(RawKey key) const;
    const std::string& getTextInputThisFrame() const;

    bool isMouseButtonDown(RawMouseButton button) const;
    bool wasMouseButtonPressed(RawMouseButton button) const;
    bool wasMouseButtonReleased(RawMouseButton button) const;

    int getMouseX() const;
    int getMouseY() const;

    const std::vector<RawTouch>& getTouches() const;

    void debugPrintState() const;

private:
    RawInputSystem() = default;

    RawTouch* findTouch(int touchId);
    const RawTouch* findTouch(int touchId) const;

    std::unordered_map<RawKey, bool> currentKeys;
    std::unordered_map<RawKey, bool> previousKeys;

    std::unordered_map<RawMouseButton, bool> currentMouseButtons;
    std::unordered_map<RawMouseButton, bool> previousMouseButtons;

    int mouseX = 0;
    int mouseY = 0;
    std::string textInputThisFrame;

    std::vector<RawTouch> touches;
};
