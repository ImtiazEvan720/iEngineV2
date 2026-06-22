#pragma once

#include <chrono>
#include <memory>

class IGuiBackend;
class IRenderBackend;
class IWindowBackend;

class Application {
public:
    Application();
    ~Application();

    bool initialize(int argc, char* argv[]);
    void tick();
    void shutdown();
    bool isRunning() const;

    Application(const Application& other) = delete;
    Application& operator=(const Application& other) = delete;
    Application(Application&& other) = delete;
    Application& operator=(Application&& other) = delete;

private:
    std::unique_ptr<IWindowBackend> windowBackend;
    std::unique_ptr<IRenderBackend> renderBackend;
    std::unique_ptr<IGuiBackend> guiBackend;
    std::chrono::steady_clock::time_point previousTime;
    bool initialized = false;
};
