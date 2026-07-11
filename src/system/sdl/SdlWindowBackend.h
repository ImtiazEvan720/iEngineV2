#pragma once

#include "system/IWindowBackend.h"

struct SDL_Renderer;
struct SDL_Window;

class SdlWindowBackend : public IWindowBackend {
public:
    SdlWindowBackend() = default;
    ~SdlWindowBackend() override = default;

    bool initialize(int width, int height, const std::string& title, int framerateLimit) override;
    void shutdown() override;

    bool isOpen() const override;
    void close() override;
    void pollEvents(InputSystem& inputSystem, IGuiBackend* guiBackend) override;

    void beginFrame(const RenderColor& clearColor) override;
    void endFrame() override;
    RenderRect getViewport() const override;
    float consumePendingPinchZoomFactor() override;
    void startTextInput() override;
    void stopTextInput() override;

    SDL_Window* getWindow();
    SDL_Renderer* getRenderer();

private:
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    bool open = false;
    int frameDelayMilliseconds = 0;
    float pendingPinchZoomFactor = 1.0f;
};
