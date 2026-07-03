#include "editor/components/AnimationComponentDrawer.h"

#include "Entity.h"
#include "components/AnimationComponent.h"
#include "misc/Animation.h"
#include "misc/AnimationLoader.h"
#include "system/ProjectManager.h"

#include "imgui.h"

#include <algorithm>
#include <filesystem>

namespace {
bool hasAnimationFileExtension(const std::filesystem::path& path) {
    return path.extension().string() == ".ianim";
}

std::string makeAnimationFileLabel(const std::filesystem::path& animationPath) {
    const std::filesystem::path animationsRoot =
        ProjectManager::getInstance().getAssetsPath() / "Animations";

    std::error_code error;
    const std::filesystem::path relativePath =
        std::filesystem::relative(animationPath, animationsRoot, error);

    if (!error && !relativePath.empty()) {
        return relativePath.string();
    }

    return animationPath.filename().string();
}

std::string makeAnimationComponentPath(const std::filesystem::path& animationPath) {
    const std::filesystem::path assetsRoot = ProjectManager::getInstance().getAssetsPath();

    std::error_code error;
    const std::filesystem::path relativePath =
        std::filesystem::relative(animationPath, assetsRoot, error);

    if (!error && !relativePath.empty()) {
        return (std::filesystem::path("Assets") / relativePath).lexically_normal().string();
    }

    return animationPath.lexically_normal().string();
}

int findAnimationFileIndex(
    const std::vector<std::filesystem::path>& animationFilePaths,
    const std::string& sourcePath
) {
    if (sourcePath.empty()) {
        return -1;
    }

    const std::filesystem::path source(sourcePath);
    const std::string sourceFilename = source.filename().string();

    for (std::size_t index = 0; index < animationFilePaths.size(); ++index) {
        const std::filesystem::path& path = animationFilePaths[index];
        if (path == source || path.filename().string() == sourceFilename) {
            return static_cast<int>(index);
        }
    }

    return -1;
}
}

void AnimationComponentDrawer::refreshAnimationFiles() {
    animationFileLabels.clear();
    animationFilePaths.clear();

    const std::filesystem::path animationsRoot =
        ProjectManager::getInstance().getAssetsPath() / "Animations";

    std::error_code error;
    if (!std::filesystem::exists(animationsRoot, error)
        || !std::filesystem::is_directory(animationsRoot, error)) {
        selectedAnimationFileIndex = -1;
        animationFilesScanned = true;
        return;
    }

    std::vector<std::filesystem::path> paths;
    for (const std::filesystem::directory_entry& entry :
         std::filesystem::recursive_directory_iterator(animationsRoot, error)) {
        if (error) {
            break;
        }

        if (entry.is_regular_file(error) && hasAnimationFileExtension(entry.path())) {
            paths.push_back(entry.path());
        }
    }

    std::sort(paths.begin(), paths.end());

    for (const std::filesystem::path& path : paths) {
        animationFilePaths.push_back(path);
        animationFileLabels.push_back(makeAnimationFileLabel(path));
    }

    if (selectedAnimationFileIndex >= static_cast<int>(animationFilePaths.size())) {
        selectedAnimationFileIndex = static_cast<int>(animationFilePaths.size()) - 1;
    }

    animationFilesScanned = true;
}

bool AnimationComponentDrawer::loadAnimationFileIntoComponent(
    const std::filesystem::path& animationPath,
    AnimationComponent& animationComponent,
    std::string& statusMessage
) {
    Animation loadedAnimation;
    if (!AnimationLoader::loadFromFile(animationPath.string(), loadedAnimation, statusMessage)) {
        return false;
    }

    animationComponent.setAnimation(loadedAnimation);
    animationComponent.setAnimationSourcePath(makeAnimationComponentPath(animationPath));

    statusMessage =
        "Loaded AnimationComponent from "
        + animationPath.filename().string()
        + " with "
        + std::to_string(loadedAnimation.getFrameCount())
        + " frame(s).";

    return true;
}

void AnimationComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* animationComponent = dynamic_cast<AnimationComponent*>(&component);
    if (animationComponent == nullptr) {
        ImGui::TextDisabled("Invalid AnimationComponent.");
        return;
    }

    Animation& animation = animationComponent->getAnimation();
    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        editState.frameDuration = animation.getFrameDuration();
        editState.playing = animationComponent->isPlaying();
        editState.looping = animationComponent->isLooping();
        selectedAnimationFileIndex = -1;
    }

    if (!animationFilesScanned) {
        refreshAnimationFiles();
    }

    if (ImGui::TreeNodeEx(
            "Animation File",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        if (selectedAnimationFileIndex < 0) {
            selectedAnimationFileIndex = findAnimationFileIndex(
                animationFilePaths,
                animationComponent->getAnimationSourcePath()
            );
        }

        const char* previewText = "No animation selected";
        if (selectedAnimationFileIndex >= 0
            && selectedAnimationFileIndex < static_cast<int>(animationFileLabels.size())) {
            previewText = animationFileLabels[static_cast<std::size_t>(selectedAnimationFileIndex)].c_str();
        }

        if (ImGui::BeginCombo("Animation", previewText)) {
            if (animationFileLabels.empty()) {
                ImGui::TextDisabled("No .ianim files found.");
            }

            for (std::size_t index = 0; index < animationFileLabels.size(); ++index) {
                const bool selected = selectedAnimationFileIndex == static_cast<int>(index);
                if (ImGui::Selectable(animationFileLabels[index].c_str(), selected)) {
                    selectedAnimationFileIndex = static_cast<int>(index);
                    if (loadAnimationFileIntoComponent(
                            animationFilePaths[index],
                            *animationComponent,
                            statusMessage)) {
                        editState.frameDuration = animationComponent->getAnimation().getFrameDuration();
                        editState.playing = animationComponent->isPlaying();
                        editState.looping = animationComponent->isLooping();
                    }
                }

                if (selected) {
                    ImGui::SetItemDefaultFocus();
                }
            }

            ImGui::EndCombo();
        }

        if (ImGui::Button("Refresh Animations")) {
            refreshAnimationFiles();
            selectedAnimationFileIndex = findAnimationFileIndex(
                animationFilePaths,
                animationComponent->getAnimationSourcePath()
            );
            statusMessage = "Refreshed animation files.";
        }

        ImGui::SameLine();
        const bool canApply =
            selectedAnimationFileIndex >= 0
            && selectedAnimationFileIndex < static_cast<int>(animationFilePaths.size());

        if (!canApply) {
            ImGui::BeginDisabled();
        }

        if (ImGui::Button("Apply Animation")) {
            if (loadAnimationFileIntoComponent(
                    animationFilePaths[static_cast<std::size_t>(selectedAnimationFileIndex)],
                    *animationComponent,
                    statusMessage)) {
                editState.frameDuration = animationComponent->getAnimation().getFrameDuration();
                editState.playing = animationComponent->isPlaying();
                editState.looping = animationComponent->isLooping();
            }
        }

        if (!canApply) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    ImGui::Text("Frames: %zu", animation.getFrameCount());
    ImGui::Text("Current Frame: %zu", animationComponent->getCurrentFrameIndex());
    ImGui::Text("Finished: %s", animationComponent->isFinished() ? "true" : "false");

    ImGui::InputFloat("Frame Duration", &editState.frameDuration, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        editState.frameDuration = std::max(0.001f, editState.frameDuration);
        animation.setFrameDuration(editState.frameDuration);
        statusMessage = "Updated AnimationComponent frame duration.";
    }

    if (ImGui::Checkbox("Playing", &editState.playing)) {
        if (editState.playing) {
            animationComponent->play();
        } else {
            animationComponent->pause();
        }

        statusMessage = "Updated AnimationComponent playback.";
    }

    if (ImGui::Checkbox("Looping", &editState.looping)) {
        animationComponent->setLooping(editState.looping);
        statusMessage = "Updated AnimationComponent looping.";
    }
}
