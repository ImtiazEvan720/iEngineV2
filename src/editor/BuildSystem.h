#pragma once

#include <mutex>
#include <string>
#include <thread>

class BuildSystem {
public:
    enum class State {
        Idle,
        Running,
        Succeeded,
        Failed,
        Unsupported
    };

    static BuildSystem& getInstance();

    BuildSystem(const BuildSystem& other) = delete;
    BuildSystem& operator=(const BuildSystem& other) = delete;
    ~BuildSystem();

    bool run(const std::string& command);
    void clearOutput();

    bool isRunning() const;
    State getState() const;
    int getExitCode() const;
    std::string getCommand() const;
    std::string getOutput() const;
    std::string getStatusText() const;

private:
    BuildSystem() = default;

    void runWorker(std::string command);
    void appendOutput(const std::string& text);
    void finish(State resultState, int resultExitCode);

    mutable std::mutex mutex;
    std::thread worker;
    State state = State::Idle;
    bool running = false;
    int exitCode = 0;
    std::string activeCommand;
    std::string output;
};
