#ifndef IENGINEV2_IGUIBACKEND_H
#define IENGINEV2_IGUIBACKEND_H

class InputSystem;
class IWindowBackend;

class IGuiBackend {
public:
    virtual ~IGuiBackend() = default;

    virtual bool initialize(IWindowBackend& windowBackend) = 0;
    virtual void processNativeEvent(const void* event) = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render(const InputSystem& inputSystem) = 0;
    virtual void shutdown() = 0;
};

#endif
