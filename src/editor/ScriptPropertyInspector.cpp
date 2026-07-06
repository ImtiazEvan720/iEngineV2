#include "editor/ScriptPropertyInspector.h"

#include "Entity.h"
#include "components/ScriptComponent.h"
#include "editor/EditorCollectionViews.h"
#include "editor/ScriptPropertyParser.h"
#include "math/Vector2F.h"
#include "misc/Level.h"
#include "misc/PrefabAsset.h"
#include "system/AssetManager.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

#include <algorithm>
#include <string>
#include <utility>
#include <vector>

namespace {
constexpr const char* EntityParentDragPayloadType = "IENGINE_ENTITY_PARENT";
constexpr const char* EntityReorderDragPayloadType = "IENGINE_ENTITY_REORDER";

std::vector<std::string> getPrefabAssetNames() {
    std::vector<std::string> prefabNames;
    const std::vector<PrefabAsset*> prefabAssets = AssetManager::getInstance().getPrefabAssets();

    for (const PrefabAsset* prefabAsset : prefabAssets) {
        if (prefabAsset != nullptr && prefabAsset->isLoaded()) {
            prefabNames.push_back(prefabAsset->getName());
        }
    }

    std::sort(prefabNames.begin(), prefabNames.end());
    return prefabNames;
}

std::string makeEntityReferenceLabel(const Entity& entity) {
    std::string label = entity.getName();
    if (!entity.getTag().empty()) {
        label += " [" + entity.getTag() + "]";
    }

    const std::string& guid = entity.getGuid();
    if (!guid.empty()) {
        label += " (" + guid.substr(0, std::min<std::size_t>(8, guid.size())) + ")";
    }

    return label;
}

std::string getEntityReferencePreview(const std::string& guid) {
    if (guid.empty()) {
        return "None";
    }

    const Entity* entity = Level::getCurrentLevel().getEntityByGuid(guid);
    if (entity == nullptr) {
        return "Missing entity";
    }

    return makeEntityReferenceLabel(*entity);
}

bool acceptEntityReferenceDrop(std::string& value, std::string& statusMessage) {
    if (!ImGui::BeginDragDropTarget()) {
        return false;
    }

    const ImGuiPayload* payload = ImGui::AcceptDragDropPayload(EntityParentDragPayloadType);
    if (payload == nullptr) {
        payload = ImGui::AcceptDragDropPayload(EntityReorderDragPayloadType);
    }

    bool accepted = false;
    if (payload != nullptr && payload->IsDelivery() && payload->DataSize == sizeof(int)) {
        const int draggedEntityId = *static_cast<const int*>(payload->Data);
        Entity* draggedEntity = Level::getCurrentLevel().findEntityById(draggedEntityId);
        if (draggedEntity != nullptr) {
            value = draggedEntity->getGuid();
            statusMessage = "Updated script entity property: " + draggedEntity->getName() + ".";
            accepted = true;
        }
    }

    ImGui::EndDragDropTarget();
    return accepted;
}

bool drawEntityReferenceValue(std::string& value, std::string& statusMessage) {
    bool changed = false;
    const std::string preview = getEntityReferencePreview(value);

    ImGui::SetNextItemWidth(280.0f);
    if (ImGui::BeginCombo("Value", preview.c_str())) {
        std::vector<ListViewItem> items;
        std::vector<std::string> guids;

        items.push_back(ListViewItem{"none", "None"});
        guids.emplace_back();
        int selectedIndex = value.empty() ? 0 : -1;

        for (Entity& levelEntity : Level::getCurrentLevel().getEntities()) {
            if (levelEntity.isDestroyed()) {
                continue;
            }

            const int itemIndex = static_cast<int>(items.size());
            if (value == levelEntity.getGuid()) {
                selectedIndex = itemIndex;
            }

            items.push_back(ListViewItem{
                levelEntity.getGuid(),
                makeEntityReferenceLabel(levelEntity)
            });
            guids.push_back(levelEntity.getGuid());
        }

        ListViewOptions options;
        options.height = 240.0f;
        options.border = false;
        options.closePopupOnSelection = true;
        options.emptyText = "No entities.";
        const ListViewResult result = EditorCollectionViews::drawListView(
            "##EntityReferenceList",
            items,
            selectedIndex,
            options
        );
        if (result.selectionChanged
            && result.clickedIndex >= 0
            && result.clickedIndex < static_cast<int>(guids.size())) {
            value = guids[static_cast<std::size_t>(result.clickedIndex)];
            statusMessage = value.empty()
                ? "Cleared script entity property."
                : "Updated script entity property.";
            changed = true;
        }

        ImGui::EndCombo();
    }

    changed = acceptEntityReferenceDrop(value, statusMessage) || changed;

    if (!value.empty() && Level::getCurrentLevel().getEntityByGuid(value) == nullptr) {
        ImGui::TextDisabled("Stored GUID does not match a loaded entity.");
    }

    return changed;
}

bool drawPrefabNameValue(
    const char* label,
    std::string& value,
    const char* updatedMessage,
    std::string& statusMessage
) {
    bool changed = false;
    const std::vector<std::string> prefabNames = getPrefabAssetNames();
    const char* preview = value.empty()
        ? "Select prefab"
        : value.c_str();

    ImGui::SetNextItemWidth(240.0f);
    if (ImGui::BeginCombo(label, preview)) {
        std::vector<ListViewItem> items;
        items.reserve(prefabNames.size());
        int selectedIndex = -1;

        for (std::size_t index = 0; index < prefabNames.size(); ++index) {
            const std::string& prefabName = prefabNames[index];
            if (value == prefabName) {
                selectedIndex = static_cast<int>(index);
            }

            items.push_back(ListViewItem{prefabName, prefabName});
        }

        ListViewOptions options;
        options.height = 220.0f;
        options.border = false;
        options.closePopupOnSelection = true;
        options.emptyText = "No prefab assets loaded.";
        const ListViewResult result = EditorCollectionViews::drawListView(
            "##PrefabNameList",
            items,
            selectedIndex,
            options
        );
        if (result.selectionChanged
            && result.clickedIndex >= 0
            && result.clickedIndex < static_cast<int>(prefabNames.size())) {
            value = prefabNames[static_cast<std::size_t>(result.clickedIndex)];
            statusMessage = updatedMessage;
            changed = true;
        }

        ImGui::EndCombo();
    }

    if (prefabNames.empty()) {
        ImGui::TextDisabled("No prefab assets loaded.");
    }

    return changed;
}

bool drawScriptValueEditor(
    const char* label,
    ScriptValue& value,
    ScriptValueType type,
    std::string& statusMessage
) {
    value.type = type;

    switch (type) {
        case ScriptValueType::Int:
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputInt(label, &value.intValue)) {
                statusMessage = "Updated script array/map integer value.";
                return true;
            }
            break;
        case ScriptValueType::Float:
            ImGui::SetNextItemWidth(160.0f);
            if (ImGui::InputFloat(label, &value.floatValue, 0.0f, 0.0f, "%.3f")) {
                statusMessage = "Updated script array/map float value.";
                return true;
            }
            break;
        case ScriptValueType::Bool:
            if (ImGui::Checkbox(label, &value.boolValue)) {
                statusMessage = "Updated script array/map bool value.";
                return true;
            }
            break;
        case ScriptValueType::Vector2: {
            float vectorValues[2] = {value.vector2Value.x, value.vector2Value.y};
            ImGui::SetNextItemWidth(220.0f);
            if (ImGui::InputFloat2(label, vectorValues, "%.2f")) {
                value.vector2Value = Vector2F(vectorValues[0], vectorValues[1]);
                statusMessage = "Updated script array/map vector2 value.";
                return true;
            }
            break;
        }
        case ScriptValueType::Entity:
            return drawEntityReferenceValue(value.stringValue, statusMessage);
        case ScriptValueType::Prefab:
            return drawPrefabNameValue(
                label,
                value.stringValue,
                "Updated script array/map prefab value.",
                statusMessage
            );
        case ScriptValueType::String:
        default:
            ImGui::SetNextItemWidth(240.0f);
            if (ImGui::InputText(label, &value.stringValue)) {
                statusMessage = "Updated script array/map string value.";
                return true;
            }
            break;
    }

    return false;
}

ScriptValue makeDefaultScriptValue(ScriptValueType type) {
    ScriptValue value;
    value.type = type;
    return value;
}
}

void ScriptPropertyInspector::draw(ScriptComponent& scriptComponent, std::string& statusMessage) {
    ScriptPropertyParser::syncWithDefinitions(scriptComponent);

    ImGui::TextWrapped("Path: %s", scriptComponent.getScriptPath().c_str());
    ImGui::Text("Loaded: %s", scriptComponent.isLoaded() ? "true" : "false");

    ImGui::Spacing();
    ImGui::TextUnformatted("Properties");

    std::vector<ScriptProperty>& properties = scriptComponent.getProperties();

    if (properties.empty()) {
        ImGui::TextDisabled("No script properties.");
    }

    for (std::size_t index = 0; index < properties.size(); ++index) {
        ScriptProperty& property = properties[index];
        ImGui::PushID(static_cast<int>(index));
        ImGui::Separator();

        ImGui::Text("Name: %s", property.name.c_str());
        ImGui::Text("Type: %s", scriptPropertyTypeToString(property.type));

        switch (property.type) {
            case ScriptPropertyType::Int:
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::InputInt("Value", &property.intValue)) {
                    statusMessage = "Updated script integer property.";
                }
                break;
            case ScriptPropertyType::Float:
                ImGui::SetNextItemWidth(160.0f);
                if (ImGui::InputFloat("Value", &property.floatValue, 0.0f, 0.0f, "%.3f")) {
                    statusMessage = "Updated script float property.";
                }
                break;
            case ScriptPropertyType::Bool:
                if (ImGui::Checkbox("Value", &property.boolValue)) {
                    statusMessage = "Updated script bool property.";
                }
                break;
            case ScriptPropertyType::Vector2: {
                float vectorValues[2] = {property.vector2Value.x, property.vector2Value.y};
                ImGui::SetNextItemWidth(220.0f);
                if (ImGui::InputFloat2("Value", vectorValues, "%.2f")) {
                    property.vector2Value = Vector2F(vectorValues[0], vectorValues[1]);
                    statusMessage = "Updated script vector2 property.";
                }
                break;
            }
            case ScriptPropertyType::Prefab: {
                drawPrefabNameValue(
                    "Value",
                    property.stringValue,
                    "Updated script prefab property.",
                    statusMessage
                );
                break;
            }
            case ScriptPropertyType::Entity:
                drawEntityReferenceValue(property.stringValue, statusMessage);
                break;
            case ScriptPropertyType::Array: {
                ImGui::Text("Element Type: %s", scriptValueTypeToString(property.elementType));

                int removeIndex = -1;
                for (std::size_t valueIndex = 0; valueIndex < property.arrayValue.size(); ++valueIndex) {
                    ScriptValue& value = property.arrayValue[valueIndex];
                    ImGui::PushID(static_cast<int>(valueIndex));
                    ImGui::Text("[%zu]", valueIndex);
                    ImGui::SameLine();
                    drawScriptValueEditor("Value", value, property.elementType, statusMessage);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Remove")) {
                        removeIndex = static_cast<int>(valueIndex);
                    }
                    ImGui::PopID();
                }

                if (removeIndex >= 0 && removeIndex < static_cast<int>(property.arrayValue.size())) {
                    property.arrayValue.erase(property.arrayValue.begin() + removeIndex);
                    statusMessage = "Removed script array value.";
                }

                if (ImGui::Button("Add Value")) {
                    property.arrayValue.push_back(makeDefaultScriptValue(property.elementType));
                    statusMessage = "Added script array value.";
                }
                break;
            }
            case ScriptPropertyType::Map: {
                ImGui::Text("Key Type: string");
                ImGui::Text("Value Type: %s", scriptValueTypeToString(property.mapValueType));

                int removeIndex = -1;
                for (std::size_t entryIndex = 0; entryIndex < property.mapValue.size(); ++entryIndex) {
                    ScriptMapEntry& entry = property.mapValue[entryIndex];
                    ImGui::PushID(static_cast<int>(entryIndex));
                    ImGui::SetNextItemWidth(160.0f);
                    if (ImGui::InputText("Key", &entry.key)) {
                        statusMessage = "Updated script map key.";
                    }
                    ImGui::SameLine();
                    drawScriptValueEditor("Value", entry.value, property.mapValueType, statusMessage);
                    ImGui::SameLine();
                    if (ImGui::SmallButton("Remove")) {
                        removeIndex = static_cast<int>(entryIndex);
                    }
                    ImGui::PopID();
                }

                if (removeIndex >= 0 && removeIndex < static_cast<int>(property.mapValue.size())) {
                    property.mapValue.erase(property.mapValue.begin() + removeIndex);
                    statusMessage = "Removed script map entry.";
                }

                if (ImGui::Button("Add Entry")) {
                    ScriptMapEntry entry;
                    entry.key = "key";
                    entry.value = makeDefaultScriptValue(property.mapValueType);
                    property.mapValue.push_back(std::move(entry));
                    statusMessage = "Added script map entry.";
                }
                break;
            }
            case ScriptPropertyType::String:
            default:
                ImGui::SetNextItemWidth(240.0f);
                if (ImGui::InputText("Value", &property.stringValue)) {
                    statusMessage = "Updated script string property.";
                }
                break;
        }

        ImGui::PopID();
    }
}
