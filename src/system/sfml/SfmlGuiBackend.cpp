#include "system/sfml/SfmlGuiBackend.h"

#include "system/InputSystem.h"
#include "system/Renderer.h"
#include "system/sfml/SfmlWindowBackend.h"

#include "imgui.h"
#include "imgui-SFML.h"

#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/Window/Event.hpp>

bool SfmlGuiBackend::initialize(IWindowBackend& backend) {
#ifndef IENGINE_WITH_EDITOR
    (void)backend;
    initialized = true;
    return true;
#else
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
#endif
}

void SfmlGuiBackend::processNativeEvent(const void* event) {
#ifndef IENGINE_WITH_EDITOR
    (void)event;
#else
    if (!initialized || windowBackend == nullptr || event == nullptr) {
        return;
    }

    const sf::Event* sfmlEvent = static_cast<const sf::Event*>(event);
    ImGui::SFML::ProcessEvent(windowBackend->getWindow(), *sfmlEvent);
#endif
}

void SfmlGuiBackend::update(float deltaTime) {
#ifndef IENGINE_WITH_EDITOR
    (void)deltaTime;
#else
    if (!initialized || windowBackend == nullptr) {
        return;
    }

    ImGui::SFML::Update(windowBackend->getWindow(), sf::seconds(deltaTime));

    levelEditor.updateEditorOnly(deltaTime);
#endif
}

void SfmlGuiBackend::render(const InputSystem& inputSystem) {
#ifndef IENGINE_WITH_EDITOR
    (void)inputSystem;
#else
    if (!initialized || windowBackend == nullptr) {
        return;
    }

    levelEditor.draw(inputSystem, Renderer::getInstance().getViewport().width);

    ImGui::SFML::Render(windowBackend->getWindow());
#endif
}

void SfmlGuiBackend::shutdown() {
#ifdef IENGINE_WITH_EDITOR
    if (initialized) {
        ImGui::SFML::Shutdown();
    }
#endif

    initialized = false;
    windowBackend = nullptr;
}
