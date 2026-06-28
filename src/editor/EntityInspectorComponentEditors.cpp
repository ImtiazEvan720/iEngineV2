#include "editor/EntityInspectorPanel.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"
#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace {
CollisionComponent::BodyType bodyTypeFromIndex(int index) {
    switch (index) {
        case 1:
            return CollisionComponent::BodyType::Kinematic;
        case 2:
            return CollisionComponent::BodyType::Dynamic;
        case 0:
        default:
            return CollisionComponent::BodyType::Static;
    }
}

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

void EntityInspectorPanel::drawTransformComponentFields(
    TransformComponent& transform,
    std::string& statusMessage
) {
    ImGui::InputFloat2("Position", entityEditState.position, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setPosition(Vector2F(entityEditState.position[0], entityEditState.position[1]));
        statusMessage = "Updated TransformComponent position.";
    }

    ImGui::InputFloat("Rotation", &entityEditState.rotation, 0.0f, 0.0f, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        transform.setRotation(entityEditState.rotation);
        statusMessage = "Updated TransformComponent rotation.";
    }

    const Vector2F worldPosition = transform.getWorldPosition();
    ImGui::Text("World Position: %.2f, %.2f", worldPosition.x, worldPosition.y);
    ImGui::Text("World Rotation: %.2f", transform.getWorldRotation());
}

void EntityInspectorPanel::drawSpriteComponentFields(
    SpriteComponent& spriteComponent,
    std::string& statusMessage
) {
    Sprite& sprite = spriteComponent.getSprite();

    if (!spritePicker.hasScannedTilesets()) {
        spritePicker.refreshTilesets();
    }

    if (ImGui::TreeNodeEx(
            "Sprite Picker",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        if (ImGui::Button("Refresh Sprites")) {
            spritePicker.refreshTilesets();
        }

        spritePicker.drawTilesetSelector();
        if (spritePicker.drawSelectedTilesetDetails(statusMessage)) {
            const bool selectionChanged =
                spritePicker.drawTileGrid("##InspectorSpritePickerGrid", false, -1, 220.0f);
            const bool applyClicked = ImGui::Button("Apply Selected Tile");

            if (selectionChanged || applyClicked) {
                std::optional<Sprite> selectedSprite = spritePicker.createSpriteFromSelectedTile(
                    Renderer::getInstance().getRenderScale(),
                    statusMessage
                );

                if (selectedSprite.has_value()) {
                    const Sprite& replacement = *selectedSprite;
                    const RenderRect& source = replacement.getSourceRect();
                    const Vector2F currentSize = sprite.getSize();
                    const Vector2F currentOrigin = sprite.getOrigin();

                    sprite.setTextureHandle(replacement.getTextureHandle());
                    sprite.setSourceRect(source);
                    sprite.setSize(currentSize);
                    sprite.setOrigin(currentOrigin);

                    entityEditState.spriteSource[0] = source.x;
                    entityEditState.spriteSource[1] = source.y;
                    entityEditState.spriteSource[2] = source.width;
                    entityEditState.spriteSource[3] = source.height;
                    entityEditState.spriteSize[0] = currentSize.x;
                    entityEditState.spriteSize[1] = currentSize.y;
                    entityEditState.spriteOrigin[0] = currentOrigin.x;
                    entityEditState.spriteOrigin[1] = currentOrigin.y;

                    statusMessage =
                        "Updated SpriteComponent from tile id "
                        + std::to_string(spritePicker.getSelectedTileId()) + ".";
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::InputFloat4("Source Rect", entityEditState.spriteSource, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSourceRect(RenderRect{
            entityEditState.spriteSource[0],
            entityEditState.spriteSource[1],
            entityEditState.spriteSource[2],
            entityEditState.spriteSource[3]
        });
        statusMessage = "Updated SpriteComponent source rect.";
    }

    ImGui::InputFloat2("Size", entityEditState.spriteSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSize(Vector2F(entityEditState.spriteSize[0], entityEditState.spriteSize[1]));
        statusMessage = "Updated SpriteComponent size.";
    }

    ImGui::InputFloat2("Origin", entityEditState.spriteOrigin, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setOrigin(Vector2F(entityEditState.spriteOrigin[0], entityEditState.spriteOrigin[1]));
        statusMessage = "Updated SpriteComponent origin.";
    }
}

void EntityInspectorPanel::drawAnimationComponentFields(
    AnimationComponent& animationComponent,
    std::string& statusMessage
) {
    Animation& animation = animationComponent.getAnimation();

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
                animationComponent.getAnimationSourcePath()
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
                    loadAnimationFileIntoComponent(animationFilePaths[index], animationComponent, statusMessage);
                    entityEditState.animationFrameDuration =
                        animationComponent.getAnimation().getFrameDuration();
                    entityEditState.animationPlaying = animationComponent.isPlaying();
                    entityEditState.animationLooping = animationComponent.isLooping();
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
                animationComponent.getAnimationSourcePath()
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
            loadAnimationFileIntoComponent(
                animationFilePaths[static_cast<std::size_t>(selectedAnimationFileIndex)],
                animationComponent,
                statusMessage
            );
            entityEditState.animationFrameDuration =
                animationComponent.getAnimation().getFrameDuration();
            entityEditState.animationPlaying = animationComponent.isPlaying();
            entityEditState.animationLooping = animationComponent.isLooping();
        }

        if (!canApply) {
            ImGui::EndDisabled();
        }

        ImGui::TreePop();
    }

    ImGui::Text("Frames: %zu", animation.getFrameCount());
    ImGui::Text("Current Frame: %zu", animationComponent.getCurrentFrameIndex());
    ImGui::Text("Finished: %s", animationComponent.isFinished() ? "true" : "false");

    ImGui::InputFloat("Frame Duration", &entityEditState.animationFrameDuration, 0.0f, 0.0f, "%.3f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.animationFrameDuration = std::max(0.001f, entityEditState.animationFrameDuration);
        animation.setFrameDuration(entityEditState.animationFrameDuration);
        statusMessage = "Updated AnimationComponent frame duration.";
    }

    if (ImGui::Checkbox("Playing", &entityEditState.animationPlaying)) {
        if (entityEditState.animationPlaying) {
            animationComponent.play();
        } else {
            animationComponent.pause();
        }

        statusMessage = "Updated AnimationComponent playback.";
    }

    if (ImGui::Checkbox("Looping", &entityEditState.animationLooping)) {
        animationComponent.setLooping(entityEditState.animationLooping);
        statusMessage = "Updated AnimationComponent looping.";
    }
}

void EntityInspectorPanel::refreshAnimationFiles() {
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

bool EntityInspectorPanel::loadAnimationFileIntoComponent(
    const std::filesystem::path& animationPath,
    AnimationComponent& animationComponent,
    std::string& statusMessage
) {
    tinyxml2::XMLDocument document;
    const tinyxml2::XMLError loadResult = document.LoadFile(animationPath.string().c_str());
    if (loadResult != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to load animation file: " + std::string(document.ErrorStr());
        return false;
    }

    const tinyxml2::XMLElement* root = document.FirstChildElement("ianim");
    if (root == nullptr) {
        statusMessage = "Animation file is missing ianim root: " + animationPath.filename().string();
        return false;
    }

    const char* tilesetFilename = root->Attribute("tileset");
    if (tilesetFilename == nullptr || tilesetFilename[0] == '\0') {
        statusMessage = "Animation file is missing tileset reference: " + animationPath.filename().string();
        return false;
    }

    if (!animationPicker.setSelectedTilesetByFilename(tilesetFilename)) {
        statusMessage = "Animation tileset was not found: " + std::string(tilesetFilename);
        return false;
    }

    const int tilesetIndex = animationPicker.getSelectedTilesetIndex();
    const EditorTileset* tileset = animationPicker.getTileset(tilesetIndex);
    if (tileset == nullptr || tileset->tileWidth <= 0 || tileset->tileHeight <= 0) {
        statusMessage = "Animation tileset metadata is invalid: " + std::string(tilesetFilename);
        return false;
    }

    const float renderScale = Renderer::getInstance().getRenderScale();
    const float tileWidth = static_cast<float>(tileset->tileWidth) * renderScale;
    const float tileHeight = static_cast<float>(tileset->tileHeight) * renderScale;
    Animation loadedAnimation;

    for (const tinyxml2::XMLElement* frameElement = root->FirstChildElement("frame");
         frameElement != nullptr;
         frameElement = frameElement->NextSiblingElement("frame")) {
        const int durationMs = std::max(1, frameElement->IntAttribute("duration", 100));
        const int widthInTiles = std::max(1, frameElement->IntAttribute("width", 1));
        const int heightInTiles = std::max(1, frameElement->IntAttribute("height", 1));
        const Vector2F frameCenter(
            static_cast<float>(widthInTiles) * tileWidth * 0.5f,
            static_cast<float>(heightInTiles) * tileHeight * 0.5f
        );
        std::vector<AnimationFrameSprite> frameSprites;

        for (const tinyxml2::XMLElement* tileElement = frameElement->FirstChildElement("tile");
             tileElement != nullptr;
             tileElement = tileElement->NextSiblingElement("tile")) {
            const int tileId = tileElement->IntAttribute("id", -1);
            const int tileX = tileElement->IntAttribute("x", 0);
            const int tileY = tileElement->IntAttribute("y", 0);

            std::optional<Sprite> sprite = animationPicker.createSpriteFromTile(
                tilesetIndex,
                tileId,
                renderScale,
                statusMessage
            );
            if (!sprite.has_value()) {
                return false;
            }

            sprite->setOrigin(Vector2F::zero());
            frameSprites.emplace_back(
                *sprite,
                Vector2F(
                    static_cast<float>(tileX) * tileWidth - frameCenter.x,
                    static_cast<float>(tileY) * tileHeight - frameCenter.y
                )
            );
        }

        if (frameSprites.empty()) {
            continue;
        }

        loadedAnimation.addFrame(
            std::move(frameSprites),
            Vector2F(
                static_cast<float>(widthInTiles) * tileWidth,
                static_cast<float>(heightInTiles) * tileHeight
            ),
            static_cast<float>(durationMs) / 1000.0f
        );
    }

    if (!loadedAnimation.hasFrames()) {
        statusMessage = "Animation file has no valid frames: " + animationPath.filename().string();
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

void EntityInspectorPanel::drawCollisionComponentFields(
    CollisionComponent& collisionComponent,
    std::string& statusMessage
) {
    ImGui::InputText("Name", &entityEditState.collisionName);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent.setName(entityEditState.collisionName);
        statusMessage = "Updated CollisionComponent name.";
    }

    ImGui::InputFloat2("Offset", entityEditState.collisionOffset, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        collisionComponent.setOffset(Vector2F(
            entityEditState.collisionOffset[0],
            entityEditState.collisionOffset[1]
        ));
        statusMessage = "Updated CollisionComponent offset.";
    }

    ImGui::InputFloat2("Size", entityEditState.collisionSize, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entityEditState.collisionSize[0] = std::max(0.001f, entityEditState.collisionSize[0]);
        entityEditState.collisionSize[1] = std::max(0.001f, entityEditState.collisionSize[1]);
        collisionComponent.setSize(entityEditState.collisionSize[0], entityEditState.collisionSize[1]);
        statusMessage = "Updated CollisionComponent size.";
    }

    if (ImGui::Combo("Body Type", &entityEditState.collisionBodyType, "Static\0Kinematic\0Dynamic\0")) {
        collisionComponent.setBodyType(bodyTypeFromIndex(entityEditState.collisionBodyType));
        statusMessage = "Updated CollisionComponent body type.";
    }

    if (ImGui::Checkbox("Sensor", &entityEditState.collisionSensor)) {
        collisionComponent.setSensor(entityEditState.collisionSensor);
        statusMessage = "Updated CollisionComponent sensor.";
    }
}

void EntityInspectorPanel::drawScriptComponentFields(
    ScriptComponent& scriptComponent,
    std::string& statusMessage
) {
    scriptPropertyInspector.draw(scriptComponent, statusMessage);
}
