#pragma once

#include "system/IRenderBackend.h"

#include <string>

class IGuiBackend;
class InputSystem;

struct WindowResizeEvent {
    int width = 0;
    int height = 0;
    int pixelWidth = 0;
    int pixelHeight = 0;
};

class IWindowBackend {
public:
    virtual ~IWindowBackend() = default;

    virtual bool initialize(int width, int height, const std::string& title, int framerateLimit) = 0;
    virtual void shutdown() = 0;

    virtual bool isOpen() const = 0;
    virtual void close() = 0;
    virtual void pollEvents(InputSystem& inputSystem, IGuiBackend* guiBackend) = 0;

    virtual void beginFrame(const RenderColor& clearColor) = 0;
    virtual void endFrame() = 0;
    virtual RenderRect getViewport() const = 0;
    virtual bool consumeResizeEvent(WindowResizeEvent& resizeEvent) = 0;
    virtual float consumePendingPinchZoomFactor() { return 1.0f; }
    virtual void startTextInput() {}
    virtual void stopTextInput() {}
};
