#pragma once

#include "system/IWindowBackend.h"

#include <memory>

namespace sf {
class RenderWindow;
}

class SfmlWindowBackend : public IWindowBackend {
public:
    SfmlWindowBackend();
    ~SfmlWindowBackend() override;

    bool initialize(int width, int height, const std::string& title, int framerateLimit) override;
    void shutdown() override;

    bool isOpen() const override;
    void close() override;
    void pollEvents(InputSystem& inputSystem, IGuiBackend* guiBackend) override;

    void beginFrame(const RenderColor& clearColor) override;
    void endFrame() override;
    RenderRect getViewport() const override;
    bool consumeResizeEvent(WindowResizeEvent& resizeEvent) override;

    sf::RenderWindow& getWindow();
    const sf::RenderWindow& getWindow() const;

private:
    std::unique_ptr<sf::RenderWindow> window;
    WindowResizeEvent pendingResizeEvent;
    bool hasPendingResizeEvent = false;
};
