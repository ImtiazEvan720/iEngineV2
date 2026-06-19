#include "editor/BuildSystem.h"

#include <array>
#include <cstdio>
#include <sstream>

#if !defined(_WIN32)
#include <sys/wait.h>
#endif

namespace {
constexpr std::size_t MaxOutputCharacters = 200000;

int normalizeExitCode(int status) {
#if defined(_WIN32)
    return status;
#else
    if (WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }

    return status;
#endif
}

FILE* openProcess(const std::string& command) {
#if defined(_WIN32)
    return _popen(command.c_str(), "r");
#else
    return popen(command.c_str(), "r");
#endif
}

int closeProcess(FILE* process) {
#if defined(_WIN32)
    return _pclose(process);
#else
    return pclose(process);
#endif
}
}

BuildSystem& BuildSystem::getInstance() {
    static BuildSystem instance;
    return instance;
}

BuildSystem::~BuildSystem() {
    if (worker.joinable()) {
        worker.join();
    }
}

bool BuildSystem::run(const std::string& command) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (running) {
            return false;
        }
    }

    if (worker.joinable()) {
        worker.join();
    }

    {
        std::lock_guard<std::mutex> lock(mutex);
        running = true;
        state = State::Running;
        exitCode = 0;
        activeCommand = command;
        output.clear();
        output += "$ " + command + "\n";
    }

    worker = std::thread(&BuildSystem::runWorker, this, command);
    return true;
}

void BuildSystem::clearOutput() {
    std::lock_guard<std::mutex> lock(mutex);
    if (!running) {
        output.clear();
        state = State::Idle;
        exitCode = 0;
        activeCommand.clear();
    }
}

bool BuildSystem::isRunning() const {
    std::lock_guard<std::mutex> lock(mutex);
    return running;
}

BuildSystem::State BuildSystem::getState() const {
    std::lock_guard<std::mutex> lock(mutex);
    return state;
}

int BuildSystem::getExitCode() const {
    std::lock_guard<std::mutex> lock(mutex);
    return exitCode;
}

std::string BuildSystem::getCommand() const {
    std::lock_guard<std::mutex> lock(mutex);
    return activeCommand;
}

std::string BuildSystem::getOutput() const {
    std::lock_guard<std::mutex> lock(mutex);
    return output;
}

std::string BuildSystem::getStatusText() const {
    std::lock_guard<std::mutex> lock(mutex);

    switch (state) {
        case State::Running:
            return "Running";
        case State::Succeeded:
            return "Succeeded";
        case State::Failed: {
            std::ostringstream stream;
            stream << "Failed (" << exitCode << ")";
            return stream.str();
        }
        case State::Unsupported:
            return "Unsupported";
        case State::Idle:
        default:
            return "Idle";
    }
}

void BuildSystem::runWorker(std::string command) {
#if defined(__EMSCRIPTEN__) || defined(IENGINE_IOS) || defined(IENGINE_ANDROID)
    (void)command;
    appendOutput("Build commands are not supported on this platform.\n");
    finish(State::Unsupported, -1);
#else
    command += " 2>&1";
    FILE* process = openProcess(command);
    if (process == nullptr) {
        appendOutput("Failed to start build process.\n");
        finish(State::Failed, -1);
        return;
    }

    std::array<char, 512> buffer{};
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), process) != nullptr) {
        appendOutput(buffer.data());
    }

    const int status = closeProcess(process);
    const int resultExitCode = normalizeExitCode(status);
    finish(resultExitCode == 0 ? State::Succeeded : State::Failed, resultExitCode);
#endif
}

void BuildSystem::appendOutput(const std::string& text) {
    std::lock_guard<std::mutex> lock(mutex);
    output += text;

    if (output.size() > MaxOutputCharacters) {
        output.erase(0, output.size() - MaxOutputCharacters);
    }
}

void BuildSystem::finish(State resultState, int resultExitCode) {
    std::lock_guard<std::mutex> lock(mutex);
    state = resultState;
    exitCode = resultExitCode;
    running = false;

    if (resultState == State::Succeeded) {
        output += "\nBuild succeeded.\n";
    } else if (resultState == State::Failed) {
        output += "\nBuild failed.\n";
    }
}
