#ifndef IENGINEV2_SFMLGUIBACKEND_H
#define IENGINEV2_SFMLGUIBACKEND_H

#include "system/IGuiBackend.h"

class SfmlWindowBackend;

class SfmlGuiBackend : public IGuiBackend {
public:
    SfmlGuiBackend() = default;
    ~SfmlGuiBackend() override = default;

    bool initialize(IWindowBackend& windowBackend) override;
    void processNativeEvent(const void* event) override;
    void update(float deltaTime) override;
    void render(const InputSystem& inputSystem) override;
    void shutdown() override;

private:
    SfmlWindowBackend* windowBackend = nullptr;
    bool initialized = false;
};

#endif
