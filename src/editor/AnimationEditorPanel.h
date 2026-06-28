
#pragma once

#include "editor/SpritePickerWidget.h"

#include <filesystem>
#include <string>
#include <vector>

struct TileInfo {
    int tileId = -1;
    int x = 0;
    int y = 0;
};

struct AnimationFrame {
    std::vector<TileInfo> tiles;
    int widthInTiles = 1;
    int heightInTiles = 1;
    int durationMs = 100;
};

class AnimationEditorPanel {
public:
    void draw(std::string& statusMessage);

private:
    void refreshAnimationList();
    bool loadAnimation(const std::filesystem::path& filePath, std::string& statusMessage);
    void drawAnimationSelector(std::string& statusMessage);
    void drawAnimationFields();
    void drawTilePicker(std::string& statusMessage);
    void drawCurrentFrameEditor(std::string& statusMessage);
    void drawFrameList(std::string& statusMessage);
    void drawFrameSlots(
        const char* id,
        AnimationFrame& frame,
        bool editable,
        std::string& statusMessage
    );

    void drawAnimationPreview(std::string& statusMessage);
    bool saveAnimation(std::string& statusMessage);
    void addCurrentFrame(std::string& statusMessage);
    void clampFrame(AnimationFrame& frame);
    void clearTilesOutsideFrame(AnimationFrame& frame);
    TileInfo* findTileAt(AnimationFrame& frame, int x, int y);
    const TileInfo* findTileAt(const AnimationFrame& frame, int x, int y) const;
    void setTileAt(AnimationFrame& frame, int x, int y, int tileId);
    void clearTileAt(AnimationFrame& frame, int x, int y);

    std::vector<std::string> availableAnimations;
    std::vector<std::filesystem::path> availableAnimationPaths;
    SpritePickerWidget spritePickerWidget;
    std::string animationName = "New Animation";
    std::string outputFile = "new_animation.ianim";
    int defaultFrameDurationMs = 100;

    AnimationFrame currentFrame;
    std::vector<AnimationFrame> animationFrames;

    int selectedFrameIndex = -1;
    int selectedAnimationIndex = -1;
    int selectedSlotX = 0;
    int selectedSlotY = 0;
    bool animationsScanned = false;
    int previewCurrentFrame = 0;
    float previewElapsedSeconds = 0.0f;
    bool previewEnabled = false;
};
