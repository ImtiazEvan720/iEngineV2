#include "editor/EntityInspectorPanel.h"

#include "components/AnimationComponent.h"
#include "components/CollisionComponent.h"
#include "components/ScriptComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Sprite.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <optional>
#include <string>

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

    if (!animationPicker.hasScannedTilesets()) {
        animationPicker.refreshTilesets();
    }

    if (ImGui::TreeNodeEx(
            "Animation Picker",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        if (ImGui::Button("Refresh Animations")) {
            animationPicker.refreshTilesets();
        }

        animationPicker.drawTilesetSelector();
        if (animationPicker.drawSelectedTilesetDetails(statusMessage)) {
            const bool selectionChanged =
                animationPicker.drawTileGrid("##InspectorAnimationPickerGrid", false, -1, 220.0f, true);
            const bool applyClicked = ImGui::Button("Apply Selected Animation");

            if (selectionChanged || applyClicked) {
                std::optional<Animation> selectedAnimation = animationPicker.createAnimationFromSelectedTile(
                    Renderer::getInstance().getRenderScale(),
                    statusMessage
                );

                if (selectedAnimation.has_value()) {
                    animationComponent.setAnimation(*selectedAnimation);
                    entityEditState.animationFrameDuration =
                        animationComponent.getAnimation().getFrameDuration();
                    entityEditState.animationPlaying = animationComponent.isPlaying();

                    statusMessage =
                        "Updated AnimationComponent from animated tile id "
                        + std::to_string(animationPicker.getSelectedTileId())
                        + " with "
                        + std::to_string(animationComponent.getAnimation().getFrameCount())
                        + " frame(s).";
                }
            }
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
