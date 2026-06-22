#pragma once

class EngineState {
public:
    enum class RuntimeMode {
        Edit,
        Play,
        Pause
    };

    static EngineState& getInstance();

    RuntimeMode getRuntimeMode() const;
    void setRuntimeMode(RuntimeMode value);
    bool isPlaying() const;
    bool isGamePaused() const;
    void setGamePaused(bool value);

    EngineState(const EngineState& other) = delete;
    EngineState& operator=(const EngineState& other) = delete;
    EngineState(EngineState&& other) = delete;
    EngineState& operator=(EngineState&& other) = delete;

private:
    EngineState() = default;

    RuntimeMode runtimeMode = RuntimeMode::Edit;
};
