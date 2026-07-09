#include "editor/SpritePalettePanel.h"

#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/Renderer.h"

#include "imgui.h"

#include <optional>

void SpritePalettePanel::draw(std::string& statusMessage) {
    if (!spritePicker.hasScannedTilesets()) {
        refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets")) {
        refreshTilesets();
    }

    spritePicker.drawTilesetSelector();
    drawSelectedTilesetGrid(statusMessage);
}

void SpritePalettePanel::refreshTilesets() {
    spritePicker.refreshTilesets();
}

void SpritePalettePanel::drawSelectedTilesetGrid(std::string& statusMessage) {
    EditorTileset* tileset = spritePicker.getSelectedTileset();
    if (tileset == nullptr) {
        return;
    }

    if (!spritePicker.drawSelectedTilesetDetails(statusMessage)) {
        return;
    }

    spritePicker.drawTileGrid("##SpritePaletteTilesetGrid", true);
}

void SpritePalettePanel::drawLevelDropTarget(
    const ViewportGrid& viewportGrid,
    const RenderRect& viewport,
    float toolbarHeight,
    std::string& statusMessage
) {
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload == nullptr || !activePayload->IsDataType(EditorTileDrag::PayloadType)) {
        return;
    }

    const ImGuiIO& io = ImGui::GetIO();
    const ImVec2 displaySize = io.DisplaySize;
    if (displaySize.x <= 0.0f || displaySize.y <= 0.0f) {
        return;
    }

    ImGui::SetNextWindowPos(ImVec2(0.0f, 0.0f), ImGuiCond_Always);
    ImGui::SetNextWindowSize(displaySize, ImGuiCond_Always);

    const ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoBackground |
        ImGuiWindowFlags_NoBringToFrontOnFocus |
        ImGuiWindowFlags_NoScrollbar |
        ImGuiWindowFlags_NoScrollWithMouse;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("##LevelDropTarget", nullptr, flags);
    ImGui::InvisibleButton("##LevelDropArea", displaySize);

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorTileDrag::PayloadType);
        if (payload != nullptr && payload->IsDelivery() && payload->DataSize == sizeof(TileDragPayload)) {
            const auto* tilePayload = static_cast<const TileDragPayload*>(payload->Data);
            Vector2F dropPosition = Renderer::getInstance().getCamera().screenToWorld(
                Vector2F(io.MousePos.x, io.MousePos.y),
                viewport
            );
            if (viewportGrid.shouldSnapToGrid()) {
                dropPosition = viewportGrid.snapPosition(dropPosition);
            }

            createSpriteEntityFromTile(
                tilePayload->tilesetIndex,
                tilePayload->tileId,
                dropPosition.x,
                dropPosition.y,
                statusMessage
            );
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(12.0f, toolbarHeight + 8.0f),
        IM_COL32(255, 255, 255, 220),
        "Drop tile to create a sprite entity"
    );
}

void SpritePalettePanel::createSpriteEntityFromTile(
    int tilesetIndex,
    int tileId,
    float x,
    float y,
    std::string& statusMessage
) {
    const EditorTileset* tileset = spritePicker.getTileset(tilesetIndex);
    if (tileset == nullptr) {
        statusMessage = "Failed to create sprite entity: invalid tileset.";
        return;
    }

    if (tileId < 0 || tileId >= tileset->tileCount) {
        statusMessage = "Failed to create sprite entity: invalid tile id.";
        return;
    }

    const float renderScale = Renderer::getInstance().getRenderScale();

    Entity& entity = Level::getCurrentLevel().createEntity();
    entity.addComponent<TransformComponent>(Vector2F(x, y), 0.0f);

    std::optional<Sprite> sprite =
        spritePicker.createSpriteFromTile(tilesetIndex, tileId, renderScale, statusMessage);
    if (!sprite.has_value()) {
        Level::getCurrentLevel().destroyEntity(entity);
        return;
    }

    entity.setName("EditorSprite_" + std::to_string(createdSpriteCount));
    entity.setTag("EditorSprite");
    entity.addComponent<SpriteComponent>(*sprite);
    statusMessage = "Created sprite entity from tile id " + std::to_string(tileId) + ".";

    ++createdSpriteCount;
    spritePicker.setSelectedTileId(tileId);
}
