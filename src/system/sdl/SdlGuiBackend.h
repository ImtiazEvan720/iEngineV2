#ifndef IENGINEV2_SDLGUIBACKEND_H
#define IENGINEV2_SDLGUIBACKEND_H

#include "system/IGuiBackend.h"

#ifdef IENGINE_WITH_EDITOR
#include "editor/LevelEditor.h"
#endif

class SdlWindowBackend;

class SdlGuiBackend : public IGuiBackend {
public:
    SdlGuiBackend() = default;
    ~SdlGuiBackend() override = default;

    bool initialize(IWindowBackend& windowBackend) override;
    void processNativeEvent(const void* event) override;
    void update(float deltaTime) override;
    void render(const InputSystem& inputSystem) override;
    void shutdown() override;

private:
    SdlWindowBackend* windowBackend = nullptr;
#ifdef IENGINE_WITH_EDITOR
    LevelEditor levelEditor;
#endif
    bool initialized = false;
};

#endif
