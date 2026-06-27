#include "editor/SpritePalettePanel.h"

#include "components/AnimationComponent.h"
#include "components/SpriteComponent.h"
#include "components/TransformComponent.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Animation.h"
#include "misc/Level.h"
#include "misc/Sprite.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "tinyxml2.h"

#include <algorithm>
#include <filesystem>
#include <optional>

namespace {
tinyxml2::XMLElement* findTileElementById(tinyxml2::XMLElement* tilesetRoot, int tileId) {
    for (tinyxml2::XMLElement* tile = tilesetRoot == nullptr ? nullptr : tilesetRoot->FirstChildElement("tile");
         tile != nullptr;
         tile = tile->NextSiblingElement("tile")) {
        if (tile->IntAttribute("id", -1) == tileId) {
            return tile;
        }
    }

    return nullptr;
}
}

void SpritePalettePanel::draw(std::string& statusMessage) {
    if (!spritePicker.hasScannedTilesets()) {
        refreshTilesets();
    }

    if (ImGui::Button("Refresh Tilesets")) {
        refreshTilesets();
    }

    if (spritePicker.drawTilesetSelector()) {
        animationOwnerTileId = -1;
    }
    drawSelectedTilesetGrid(statusMessage);
}

void SpritePalettePanel::refreshTilesets() {
    spritePicker.refreshTilesets();
    animationOwnerTileId = -1;
}

void SpritePalettePanel::drawSelectedTilesetGrid(std::string& statusMessage) {
    EditorTileset* tileset = spritePicker.getSelectedTileset();
    if (tileset == nullptr) {
        return;
    }

    if (!spritePicker.drawSelectedTilesetDetails(statusMessage)) {
        return;
    }

    drawSelectedTileAnimationEditor(*tileset, statusMessage);
    spritePicker.drawTileGrid("##SpritePaletteTilesetGrid", true, animationOwnerTileId);
}

void SpritePalettePanel::drawSelectedTileAnimationEditor(EditorTileset& tileset, std::string& statusMessage) {
    ImGui::Separator();

    const int selectedTileId = spritePicker.getSelectedTileId();
    if (selectedTileId < 0) {
        ImGui::TextUnformatted("Select a tile to edit animation frames.");
        return;
    }

    if (animationOwnerTileId < 0 || animationOwnerTileId >= tileset.tileCount) {
        animationOwnerTileId = selectedTileId;
    }

    ImGui::Text("Animation owner tile: %d", animationOwnerTileId);
    ImGui::SameLine();
    if (ImGui::Button("Use Selected As Owner")) {
        animationOwnerTileId = selectedTileId;
    }

    ImGui::InputInt("Owner Tile Id", &animationOwnerTileId);
    animationOwnerTileId = std::clamp(animationOwnerTileId, 0, std::max(0, tileset.tileCount - 1));

    ImGui::InputInt("Frame Duration Ms", &newAnimationFrameDurationMs);
    newAnimationFrameDurationMs = std::max(1, newAnimationFrameDurationMs);
    const float framesPerSecond = 1000.0f / static_cast<float>(newAnimationFrameDurationMs);
    ImGui::Text("Speed: %.2f FPS", framesPerSecond);

    if (ImGui::Button("Add Selected Tile As Frame")) {
        EditorTileAnimation& animation = tileset.animations[animationOwnerTileId];
        animation.frames.push_back(EditorAnimationFrame{
            selectedTileId,
            newAnimationFrameDurationMs
        });
        statusMessage =
            "Added tile " + std::to_string(selectedTileId)
            + " as animation frame for tile " + std::to_string(animationOwnerTileId) + ".";
    }

    ImGui::SameLine();

    if (ImGui::Button("Clear Animation")) {
        tileset.animations.erase(animationOwnerTileId);
        statusMessage = "Cleared animation for tile " + std::to_string(animationOwnerTileId) + ".";
    }

    auto animationIterator = tileset.animations.find(animationOwnerTileId);
    if (animationIterator == tileset.animations.end() || animationIterator->second.frames.empty()) {
        ImGui::TextUnformatted("No frames for this tile yet.");
    } else {
        EditorTileAnimation& animation = animationIterator->second;
        int removeFrameIndex = -1;

        for (std::size_t frameIndex = 0; frameIndex < animation.frames.size(); ++frameIndex) {
            EditorAnimationFrame& frame = animation.frames[frameIndex];
            ImGui::PushID(static_cast<int>(frameIndex));

            ImGui::Text("Frame %zu", frameIndex);
            ImGui::SameLine();
            ImGui::SetNextItemWidth(96.0f);
            ImGui::InputInt("Tile", &frame.tileId);
            frame.tileId = std::clamp(frame.tileId, 0, std::max(0, tileset.tileCount - 1));

            ImGui::SameLine();
            ImGui::SetNextItemWidth(112.0f);
            ImGui::InputInt("Duration", &frame.durationMs);
            frame.durationMs = std::max(1, frame.durationMs);

            ImGui::SameLine();
            if (ImGui::Button("Remove")) {
                removeFrameIndex = static_cast<int>(frameIndex);
            }

            ImGui::PopID();
        }

        if (removeFrameIndex >= 0
            && removeFrameIndex < static_cast<int>(animation.frames.size())) {
            animation.frames.erase(animation.frames.begin() + removeFrameIndex);

            if (animation.frames.empty()) {
                tileset.animations.erase(animationOwnerTileId);
            }
        }
    }

    if (ImGui::Button("Save Tileset Animations")) {
        if (saveTilesetAnimations(tileset, statusMessage)) {
            spritePicker.refreshTilesets();
        }
    }
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

    const auto animationIterator = tileset->animations.find(tileId);
    if (animationIterator != tileset->animations.end()
        && !animationIterator->second.frames.empty()) {
        Animation animation;
        bool addedFrame = false;

        for (const EditorAnimationFrame& frame : animationIterator->second.frames) {
            if (frame.tileId < 0 || frame.tileId >= tileset->tileCount) {
                continue;
            }

            std::optional<Sprite> frameSprite =
                spritePicker.createSpriteFromTile(tilesetIndex, frame.tileId, renderScale, statusMessage);
            if (!frameSprite.has_value()) {
                continue;
            }

            animation.addFrame(*frameSprite);
            if (!addedFrame) {
                animation.setFrameDuration(static_cast<float>(std::max(1, frame.durationMs)) / 1000.0f);
            }

            addedFrame = true;
        }

        if (!addedFrame) {
            statusMessage = "Failed to create animation entity: animation has no valid frames.";
            Level::getCurrentLevel().destroyEntity(entity);
            return;
        }

        entity.setName("EditorAnimation_" + std::to_string(createdSpriteCount));
        entity.setTag("EditorAnimation");
        entity.addComponent<AnimationComponent>(animation);
        statusMessage =
            "Created animation entity from owner tile id "
            + std::to_string(tileId)
            + " with " + std::to_string(animation.getFrameCount()) + " frame(s).";
    } else {
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
    }

    ++createdSpriteCount;
    spritePicker.setSelectedTileId(tileId);
}

bool SpritePalettePanel::saveTilesetAnimations(EditorTileset& tileset, std::string& statusMessage) {
    tinyxml2::XMLDocument document;
    if (document.LoadFile(tileset.path.c_str()) != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to load tileset for saving: " + std::string(document.ErrorStr());
        return false;
    }

    tinyxml2::XMLElement* tilesetRoot = document.FirstChildElement("tileset");
    if (tilesetRoot == nullptr) {
        statusMessage = "Failed to save animations: tileset root is missing.";
        return false;
    }

    for (tinyxml2::XMLElement* tile = tilesetRoot->FirstChildElement("tile");
         tile != nullptr;
         tile = tile->NextSiblingElement("tile")) {
        for (tinyxml2::XMLElement* animation = tile->FirstChildElement("animation");
             animation != nullptr;) {
            tinyxml2::XMLElement* nextAnimation = animation->NextSiblingElement("animation");
            tile->DeleteChild(animation);
            animation = nextAnimation;
        }
    }

    std::vector<int> ownerTileIds;
    ownerTileIds.reserve(tileset.animations.size());
    for (const auto& animationPair : tileset.animations) {
        if (!animationPair.second.frames.empty()) {
            ownerTileIds.push_back(animationPair.first);
        }
    }

    std::sort(ownerTileIds.begin(), ownerTileIds.end());

    for (int ownerTileId : ownerTileIds) {
        if (ownerTileId < 0 || ownerTileId >= tileset.tileCount) {
            continue;
        }

        tinyxml2::XMLElement* tile = findTileElementById(tilesetRoot, ownerTileId);
        if (tile == nullptr) {
            tile = document.NewElement("tile");
            tile->SetAttribute("id", ownerTileId);
            tilesetRoot->InsertEndChild(tile);
        }

        tinyxml2::XMLElement* animation = document.NewElement("animation");
        const EditorTileAnimation& editorAnimation = tileset.animations[ownerTileId];

        for (const EditorAnimationFrame& editorFrame : editorAnimation.frames) {
            tinyxml2::XMLElement* frame = document.NewElement("frame");
            frame->SetAttribute("tileid", std::clamp(editorFrame.tileId, 0, std::max(0, tileset.tileCount - 1)));
            frame->SetAttribute("duration", std::max(1, editorFrame.durationMs));
            animation->InsertEndChild(frame);
        }

        tile->InsertEndChild(animation);
    }

    const tinyxml2::XMLError result = document.SaveFile(tileset.path.c_str());
    if (result != tinyxml2::XML_SUCCESS) {
        statusMessage = "Failed to save tileset animations: " + std::string(document.ErrorStr());
        return false;
    }

    statusMessage = "Saved tileset animations to " + std::filesystem::path(tileset.path).string() + ".";
    return true;
}
