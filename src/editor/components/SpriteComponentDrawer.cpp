#include "editor/components/SpriteComponentDrawer.h"

#include "Entity.h"
#include "components/SpriteComponent.h"
#include "math/Vector2F.h"
#include "misc/Sprite.h"
#include "system/IRenderBackend.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <optional>

void SpriteComponentDrawer::draw(
    Entity& entity,
    Component& component,
    std::string& statusMessage
) {
    auto* spriteComponent = dynamic_cast<SpriteComponent*>(&component);
    if (spriteComponent == nullptr) {
        ImGui::TextDisabled("Invalid SpriteComponent.");
        return;
    }

    Sprite& sprite = spriteComponent->getSprite();
    if (editState.entityId != entity.getId()) {
        editState.entityId = entity.getId();
        const RenderRect& source = sprite.getSourceRect();
        const Vector2F& size = sprite.getSize();
        const Vector2F& origin = sprite.getOrigin();

        editState.source[0] = source.x;
        editState.source[1] = source.y;
        editState.source[2] = source.width;
        editState.source[3] = source.height;
        editState.size[0] = size.x;
        editState.size[1] = size.y;
        editState.origin[0] = origin.x;
        editState.origin[1] = origin.y;
    }

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
                spritePicker.drawTileGrid("##SpriteComponentDrawerGrid", false, 220.0f);
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

                    editState.source[0] = source.x;
                    editState.source[1] = source.y;
                    editState.source[2] = source.width;
                    editState.source[3] = source.height;
                    editState.size[0] = currentSize.x;
                    editState.size[1] = currentSize.y;
                    editState.origin[0] = currentOrigin.x;
                    editState.origin[1] = currentOrigin.y;

                    statusMessage =
                        "Updated SpriteComponent from tile id "
                        + std::to_string(spritePicker.getSelectedTileId()) + ".";
                }
            }
        }

        ImGui::TreePop();
    }

    ImGui::InputFloat4("Source Rect", editState.source, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSourceRect(RenderRect{
            editState.source[0],
            editState.source[1],
            editState.source[2],
            editState.source[3]
        });
        statusMessage = "Updated SpriteComponent source rect.";
    }

    ImGui::InputFloat2("Size", editState.size, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setSize(Vector2F(editState.size[0], editState.size[1]));
        statusMessage = "Updated SpriteComponent size.";
    }

    ImGui::InputFloat2("Origin", editState.origin, "%.2f");
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        sprite.setOrigin(Vector2F(editState.origin[0], editState.origin[1]));
        statusMessage = "Updated SpriteComponent origin.";
    }
}
