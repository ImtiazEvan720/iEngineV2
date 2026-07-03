#include "editor/EntityComponentList.h"

#include "Entity.h"
#include "components/Component.h"
#include "editor/components/IComponentDrawer.h"

#include "imgui.h"

#include <memory>
#include <string>
#include <vector>

namespace {
void drawComponentEnabledCheckbox(
    Component& component,
    const char* componentName,
    std::string& statusMessage
) {
    bool enabled = component.isEnabled();
    if (ImGui::Checkbox("Enabled", &enabled)) {
        component.setEnabled(enabled);
        statusMessage = std::string(componentName)
            + (enabled ? " enabled." : " disabled.");
    }
}
}

bool EntityComponentList::draw(
    Entity& entity,
    bool editable,
    AddComponentCallback addComponentCallback,
    RemoveComponentCallback removeComponentCallback,
    std::string& statusMessage
) {
    if (!ImGui::TreeNodeEx(
            "Components",
            ImGuiTreeNodeFlags_DefaultOpen |
            ImGuiTreeNodeFlags_OpenOnArrow |
            ImGuiTreeNodeFlags_SpanAvailWidth)) {
        return false;
    }

    if (editable) {
        addComponentCallback(entity, statusMessage);
        ImGui::Separator();
    }

    const std::vector<std::unique_ptr<Component>>& components = entity.getComponents();
    if (components.empty()) {
        ImGui::TextUnformatted("No known components.");
        ImGui::TreePop();
        return false;
    }

    for (const std::unique_ptr<Component>& componentOwner : components) {
        Component* component = componentOwner.get();
        if (component == nullptr) {
            continue;
        }

        const char* componentName = componentDrawerRegistry.getComponentName(*component);
        if (componentName == nullptr) {
            componentName = "UnknownComponent";
        }

        ImGui::PushID(component);
        if (ImGui::TreeNodeEx(
                componentName,
                ImGuiTreeNodeFlags_OpenOnArrow |
                ImGuiTreeNodeFlags_SpanAvailWidth)) {
            if (editable) {
                if (removeComponentCallback(entity, componentName, statusMessage)) {
                    ImGui::TreePop();
                    ImGui::PopID();
                    ImGui::TreePop();
                    return true;
                }

                drawComponentEnabledCheckbox(*component, componentName, statusMessage);

                if (IComponentDrawer* drawer = componentDrawerRegistry.getDrawer(*component)) {
                    drawer->draw(entity, *component, statusMessage);
                } else {
                    ImGui::TextDisabled("No drawer registered for this component.");
                }
            } else {
                ImGui::Text("Enabled: %s", component->isEnabled() ? "true" : "false");
                ImGui::TextDisabled("Select this entity to edit component fields.");
            }

            ImGui::TreePop();
        }

        ImGui::PopID();
    }

    ImGui::TreePop();
    return false;
}
