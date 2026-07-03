#include "editor/EntityIdentityDrawer.h"

#include "Entity.h"

#include "imgui.h"
#include "misc/cpp/imgui_stdlib.h"

void EntityIdentityDrawer::clear() {
    editState = EditState{};
}

void EntityIdentityDrawer::syncFromEntity(Entity& entity, bool force) {
    if (!force && editState.entityId == entity.getId()) {
        return;
    }

    editState = EditState{};
    editState.entityId = entity.getId();
    editState.name = entity.getName();
    editState.tag = entity.getTag();
    editState.enabled = entity.isEnabled();
}

void EntityIdentityDrawer::draw(Entity& entity, std::string& statusMessage) {
    ImGui::InputText("Name", &editState.name);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setName(editState.name);
        statusMessage = "Updated entity name.";
    }

    ImGui::InputText("Tag", &editState.tag);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setTag(editState.tag);
        statusMessage = "Updated entity tag.";
    }

    ImGui::Checkbox("Enabled", &editState.enabled);
    if (ImGui::IsItemDeactivatedAfterEdit()) {
        entity.setEnabled(editState.enabled);
        statusMessage = "Updated entity enabled state.";
    }

    if (Entity* parent = entity.getParent()) {
        ImGui::Text("Parent: %s (%d)", parent->getName().c_str(), parent->getId());
        if (ImGui::Button("Clear Parent")) {
            entity.clearParent();
            syncFromEntity(entity, true);
            statusMessage = "Cleared entity parent.";
        }
    } else {
        ImGui::TextUnformatted("Parent: none");
    }
}
