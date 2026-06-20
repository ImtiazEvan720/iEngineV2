#include "system/sfml/SfmlGuiBackend.h"

#include "system/InputSystem.h"
#include "system/sfml/SfmlWindowBackend.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

bool SfmlGuiBackend::initialize(IWindowBackend& backend) {
    windowBackend = dynamic_cast<SfmlWindowBackend*>(&backend);
    if (windowBackend == nullptr) {
        initialized = false;
        return false;
    }

    initialized = ImGui::SFML::Init(windowBackend->getWindow());
    if (initialized) {
        ImGui::GetIO().ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    }

    return initialized;
}

void SfmlGuiBackend::processNativeEvent(const void* event) {
    if (!initialized || windowBackend == nullptr || event == nullptr) {
        return;
    }

    const sf::Event* sfmlEvent = static_cast<const sf::Event*>(event);
    ImGui::SFML::ProcessEvent(windowBackend->getWindow(), *sfmlEvent);
}

void SfmlGuiBackend::update(float deltaTime) {
    if (!initialized || windowBackend == nullptr) {
        return;
    }

    ImGui::SFML::Update(windowBackend->getWindow(), sf::seconds(deltaTime));

#ifdef IENGINE_WITH_EDITOR
    levelEditor.updateEditorOnly(deltaTime);
#endif
}

void SfmlGuiBackend::render(const InputSystem& inputSystem) {
    if (!initialized || windowBackend == nullptr) {
        return;
    }

#ifdef IENGINE_WITH_EDITOR
    levelEditor.draw(inputSystem, windowBackend->getViewport().width);
#else
    (void)inputSystem;
#endif

    ImGui::SFML::Render(windowBackend->getWindow());
}

void SfmlGuiBackend::shutdown() {
    if (initialized) {
        ImGui::SFML::Shutdown();
    }

    initialized = false;
    windowBackend = nullptr;
}
