#pragma once

#include "editor/components/IComponentDrawer.h"

#include <filesystem>
#include <string>
#include <vector>

class AnimationComponentDrawer : public IComponentDrawer {
public:
    void draw(Entity& entity, Component& component, std::string& statusMessage) override;

private:
    struct EditState {
        int entityId = -1;
        float frameDuration = 0.0f;
        bool playing = false;
        bool looping = true;
    };

    void refreshAnimationFiles();
    bool loadAnimationFileIntoComponent(
        const std::filesystem::path& animationPath,
        class AnimationComponent& animationComponent,
        std::string& statusMessage
    );

    EditState editState;
    int selectedAnimationFileIndex = -1;
    bool animationFilesScanned = false;
    std::vector<std::string> animationFileLabels;
    std::vector<std::filesystem::path> animationFilePaths;
};
