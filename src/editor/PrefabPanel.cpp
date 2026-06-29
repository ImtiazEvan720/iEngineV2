#include "editor/PrefabPanel.h"

#include "Entity.h"
#include "editor/EditorCollectionViews.h"
#include "editor/EditorPrefabTypes.h"
#include "editor/ViewportGrid.h"
#include "math/Vector2F.h"
#include "misc/Camera2D.h"
#include "misc/Level.h"
#include "misc/TextureAsset.h"
#include "serialization/PrefabSerializer.h"
#include "system/AssetManager.h"
#include "system/ProjectManager.h"
#include "system/Renderer.h"

#include "imgui.h"
#include "tinyxml2.h"

#include <algorithm>
#include <cstring>
#include <filesystem>
#include <iostream>
#include <vector>

namespace {
const tinyxml2::XMLElement* findPreviewElement(const tinyxml2::XMLElement& entityElement) {
    for (const tinyxml2::XMLElement* component = entityElement.FirstChildElement("component");
         component != nullptr;
         component = component->NextSiblingElement("component")) {
        const char* type = component->Attribute("type");
        if (type == nullptr) {
            continue;
        }

        const std::string componentType(type);
        if (componentType == "SpriteComponent") {
            return component;
        }

        if (componentType == "AnimationComponent") {
            const tinyxml2::XMLElement* frame = component->FirstChildElement("frame");
            if (frame != nullptr) {
                const tinyxml2::XMLElement* sprite = frame->FirstChildElement("sprite");
                if (sprite != nullptr) {
                    return sprite;
                }

                return frame;
            }
        }
    }

    return nullptr;
}

PrefabPanel::PrefabPreview loadPrefabPreview(const std::string& prefabPath) {
    PrefabPanel::PrefabPreview preview;

    tinyxml2::XMLDocument document;
    if (document.LoadFile(prefabPath.c_str()) != tinyxml2::XML_SUCCESS) {
        return preview;
    }

    const tinyxml2::XMLElement* prefab = document.FirstChildElement("prefab");
    const tinyxml2::XMLElement* entity = prefab ? prefab->FirstChildElement("entity") : nullptr;
    if (entity == nullptr) {
        return preview;
    }

    const tinyxml2::XMLElement* visualElement = findPreviewElement(*entity);
    if (visualElement == nullptr) {
        return preview;
    }

    const char* textureName = visualElement->Attribute("texture");
    if (textureName == nullptr) {
        return preview;
    }

    TextureAsset* textureAsset =
        AssetManager::getInstance().getTextureAssetByName(textureName);

    if (textureAsset == nullptr
        || textureAsset->getImGuiTextureId() == ImTextureID{}
        || textureAsset->getWidth() <= 0
        || textureAsset->getHeight() <= 0) {
        return preview;
    }

    const float sourceX = visualElement->FloatAttribute("sourceX");
    const float sourceY = visualElement->FloatAttribute("sourceY");
    const float sourceW = visualElement->FloatAttribute("sourceWidth");
    const float sourceH = visualElement->FloatAttribute("sourceHeight");

    preview.textureId = textureAsset->getImGuiTextureId();
    preview.sourceWidth = sourceW;
    preview.sourceHeight = sourceH;
    preview.width = visualElement->FloatAttribute("sizeX", 0.0f);
    preview.height = visualElement->FloatAttribute("sizeY", 0.0f);

    preview.uv0 = ImVec2(
        sourceX / static_cast<float>(textureAsset->getWidth()),
        sourceY / static_cast<float>(textureAsset->getHeight())
    );

    preview.uv1 = ImVec2(
        (sourceX + sourceW) / static_cast<float>(textureAsset->getWidth()),
        (sourceY + sourceH) / static_cast<float>(textureAsset->getHeight())
    );
    preview.valid = true;

    return preview;
}

std::vector<unsigned char> makePrefabDragPayloadBytes(const PrefabDragPayload& payload) {
    const auto* begin = reinterpret_cast<const unsigned char*>(&payload);
    return std::vector<unsigned char>(begin, begin + sizeof(PrefabDragPayload));
}
}


void PrefabPanel::draw(std::string& statusMessage) {
    if (!prefabsScanned) {
        refreshPrefabs();
    }

    if (ImGui::Button("Refresh Prefabs")) {
        refreshPrefabs();
        statusMessage = "Refreshed prefabs.";
    }

    if (!statusMessage.empty()) {
        ImGui::TextWrapped("%s", statusMessage.c_str());
    }

    ImGui::Separator();

    if (prefabs.empty()) {
        ImGui::TextUnformatted("No prefab assets loaded.");
        return;
    }

    std::vector<GridViewItem> items;
    items.reserve(prefabs.size());
    for (const PrefabInfo& prefab : prefabs) {
        GridViewItem item;
        item.id = prefab.path;
        item.label = prefab.name;
        item.tooltip = prefab.path;

        if (prefab.preview.valid) {
            item.image = prefab.preview.textureId;
            item.imageSize = ImVec2(48.0f, 48.0f);
            item.uv0 = prefab.preview.uv0;
            item.uv1 = prefab.preview.uv1;

            Renderer& renderer = Renderer::getInstance();
            const float renderScale = renderer.getRenderScale();
            const float cameraZoom = renderer.getCamera().getZoom();
            const float previewWidth = prefab.preview.width > 0.0f
                ? prefab.preview.width
                : prefab.preview.sourceWidth * renderScale;
            const float previewHeight = prefab.preview.height > 0.0f
                ? prefab.preview.height
                : prefab.preview.sourceHeight * renderScale;

            item.dragPreviewSize = ImVec2(
                previewWidth * cameraZoom,
                previewHeight * cameraZoom
            );
        }

        PrefabDragPayload payload{};
        std::strncpy(payload.path, prefab.path.c_str(), EditorPrefabDrag::MaxPathLength - 1);
        item.dragPayloadType = EditorPrefabDrag::PayloadType;
        item.dragPayload = makePrefabDragPayloadBytes(payload);
        item.dragLabel = "Prefab: " + prefab.name;
        items.push_back(std::move(item));
    }

    GridViewOptions options;
    options.cellWidth = 88.0f;
    options.cellHeight = 84.0f;
    options.height = std::max(120.0f, ImGui::GetContentRegionAvail().y - (ImGui::GetFrameHeightWithSpacing() * 2.0f));
    options.emptyText = "No prefab assets loaded.";

    EditorCollectionViews::drawGridView(
        "##PrefabGrid",
        items,
        selectedPrefabIndex,
        options
    );

    drawDeleteSelectedPrefabButton(statusMessage);
    drawDeletePrefabConfirmPopup(statusMessage);
}

void PrefabPanel::refreshPrefabs() {
    prefabs.clear();
    selectedPrefabIndex = -1;

    std::vector<PrefabAsset*> prefabAssets = AssetManager::getInstance().getPrefabAssets();
    std::sort(
        prefabAssets.begin(),
        prefabAssets.end(),
        [](const PrefabAsset* left, const PrefabAsset* right) {
            if (left == nullptr || right == nullptr) {
                return left != nullptr;
            }

            return left->getPath() < right->getPath();
        }
    );

    for (PrefabAsset* prefabAsset : prefabAssets) {
        if (prefabAsset == nullptr || !prefabAsset->isLoaded()) {
            continue;
        }

        const std::string& prefabPath = prefabAsset->getPath();
        prefabs.push_back(PrefabInfo{
            prefabAsset->getName(),
            prefabPath,
            loadPrefabPreview(prefabPath)
        });
    }

    if (!prefabs.empty()) {
        selectedPrefabIndex = 0;
    }

    prefabsScanned = true;
}

void PrefabPanel::drawDeleteSelectedPrefabButton(std::string& statusMessage) {
    const bool hasSelection =
        selectedPrefabIndex >= 0 && selectedPrefabIndex < static_cast<int>(prefabs.size());

    if (!hasSelection) {
        ImGui::BeginDisabled();
    }

    if (ImGui::Button("Delete Selected Prefab")) {
        const PrefabInfo& prefab = prefabs[static_cast<std::size_t>(selectedPrefabIndex)];
        pendingDeletePrefabPath = prefab.path;
        pendingDeletePrefabName = prefab.name;
        deleteConfirmOpen = true;
        ImGui::OpenPopup("Delete Prefab?");
    }

    if (!hasSelection) {
        ImGui::EndDisabled();
    }

    (void)statusMessage;
}

void PrefabPanel::drawDeletePrefabConfirmPopup(std::string& statusMessage) {
    if (deleteConfirmOpen) {
        ImGui::OpenPopup("Delete Prefab?");
        deleteConfirmOpen = false;
    }

    if (!ImGui::BeginPopupModal("Delete Prefab?", nullptr, ImGuiWindowFlags_AlwaysAutoResize)) {
        return;
    }

    ImGui::Text("Delete prefab \"%s\"?", pendingDeletePrefabName.c_str());
    ImGui::TextWrapped("This deletes the .iprefab file from disk.");

    if (ImGui::Button("Delete", ImVec2(120.0f, 0.0f))) {
        deleteSelectedPrefab(statusMessage);
        pendingDeletePrefabPath.clear();
        pendingDeletePrefabName.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::SameLine();

    if (ImGui::Button("Cancel", ImVec2(120.0f, 0.0f))) {
        pendingDeletePrefabPath.clear();
        pendingDeletePrefabName.clear();
        ImGui::CloseCurrentPopup();
    }

    ImGui::EndPopup();
}

bool PrefabPanel::deleteSelectedPrefab(std::string& statusMessage) {
    if (pendingDeletePrefabPath.empty()) {
        statusMessage = "No prefab selected for deletion.";
        return false;
    }

    std::error_code error;
    const bool removed = std::filesystem::remove(pendingDeletePrefabPath, error);
    if (error) {
        statusMessage = "Failed to delete prefab: " + error.message();
        return false;
    }

    if (!removed) {
        statusMessage = "Failed to delete prefab: file was not found.";
        return false;
    }

    AssetManager::getInstance().loadAssets(ProjectManager::getInstance().getAssetsPath().string());
    refreshPrefabs();

    statusMessage = "Deleted prefab: " + pendingDeletePrefabName + ".";
    return true;
}

void PrefabPanel::drawLevelDropTarget(
    const ViewportGrid& viewportGrid,
    const RenderRect& viewport,
    float toolbarHeight,
    std::string& statusMessage
) {
    const ImGuiPayload* activePayload = ImGui::GetDragDropPayload();
    if (activePayload == nullptr || !activePayload->IsDataType(EditorPrefabDrag::PayloadType)) {
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
    ImGui::Begin("##PrefabLevelDropTarget", nullptr, flags);
    ImGui::InvisibleButton("##PrefabLevelDropArea", displaySize);

    if (ImGui::BeginDragDropTarget()) {
        const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EditorPrefabDrag::PayloadType);
        if (payload != nullptr && payload->IsDelivery() && payload->DataSize == sizeof(PrefabDragPayload)) {
            const auto* prefabPayload = static_cast<const PrefabDragPayload*>(payload->Data);
            Vector2F dropPosition = Renderer::getInstance().getCamera().screenToWorld(
                Vector2F(io.MousePos.x, io.MousePos.y),
                viewport
            );
            if (viewportGrid.shouldSnapToGrid()) {
                dropPosition = viewportGrid.snapPosition(dropPosition);
            }

            std::string errorMessage;
            Entity* entity = PrefabSerializer::instantiate(
                prefabPayload->path,
                Level::getCurrentLevel(),
                dropPosition,
                errorMessage
            );
            if (entity == nullptr) {
                statusMessage = errorMessage.empty() ? "Failed to instantiate prefab." : errorMessage;
            } else {
                statusMessage = "Instantiated prefab as entity " + std::to_string(entity->getId()) + ".";
            }
        }

        ImGui::EndDragDropTarget();
    }

    ImGui::End();
    ImGui::PopStyleVar();

    ImGui::GetForegroundDrawList()->AddText(
        ImVec2(12.0f, toolbarHeight + 28.0f),
        IM_COL32(255, 255, 255, 220),
        "Drop prefab to instantiate it"
    );
}
