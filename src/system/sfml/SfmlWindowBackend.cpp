#include "system/sfml/SfmlWindowBackend.h"

#include "system/IGuiBackend.h"
#include "system/InputSystem.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/View.hpp>
#include <SFML/Window/ContextSettings.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/Window/VideoMode.hpp>
#include <SFML/Window/WindowEnums.hpp>

#include <memory>
#include <optional>

namespace {
InputKey mapKey(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::W:
            return InputKey::W;
        case sf::Keyboard::Key::A:
            return InputKey::A;
        case sf::Keyboard::Key::S:
            return InputKey::S;
        case sf::Keyboard::Key::D:
            return InputKey::D;
        case sf::Keyboard::Key::Space:
            return InputKey::Space;
        default:
            return InputKey::Unknown;
    }
}

InputMouseButton mapMouseButton(sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left:
            return InputMouseButton::Left;
        case sf::Mouse::Button::Right:
            return InputMouseButton::Right;
        case sf::Mouse::Button::Middle:
            return InputMouseButton::Middle;
        default:
            return InputMouseButton::Unknown;
    }
}
}

SfmlWindowBackend::SfmlWindowBackend() = default;

SfmlWindowBackend::~SfmlWindowBackend() = default;

bool SfmlWindowBackend::initialize(
    int width,
    int height,
    const std::string& title,
    int framerateLimit
) {
    sf::ContextSettings settings;
    settings.depthBits = 24;
    settings.stencilBits = 8;
    settings.majorVersion = 2;
    settings.minorVersion = 1;

    window = std::make_unique<sf::RenderWindow>(
        sf::VideoMode({static_cast<unsigned int>(width), static_cast<unsigned int>(height)}),
        title,
        sf::Style::Default,
        sf::State::Windowed,
        settings
    );

    window->setFramerateLimit(static_cast<unsigned int>(framerateLimit));
    return window->isOpen();
}

void SfmlWindowBackend::shutdown() {
    if (window != nullptr && window->isOpen()) {
        window->close();
    }

    window.reset();
}

bool SfmlWindowBackend::isOpen() const {
    return window != nullptr && window->isOpen();
}

void SfmlWindowBackend::close() {
    if (window != nullptr) {
        window->close();
    }
}

void SfmlWindowBackend::pollEvents(InputSystem& inputSystem, IGuiBackend* guiBackend) {
    if (window == nullptr) {
        return;
    }

    while (const std::optional event = window->pollEvent()) {
        if (guiBackend != nullptr) {
            guiBackend->processNativeEvent(&*event);
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            inputSystem.processKeyPressed(mapKey(keyPressed->code));
        } else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            inputSystem.processKeyReleased(mapKey(keyReleased->code));
        } else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            inputSystem.processMousePressed(
                mapMouseButton(mousePressed->button),
                mousePressed->position.x,
                mousePressed->position.y
            );
        } else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            inputSystem.processMouseReleased(
                mapMouseButton(mouseReleased->button),
                mouseReleased->position.x,
                mouseReleased->position.y
            );
        }

        if (event->is<sf::Event::Closed>()) {
            window->close();
        }
    }
}

void SfmlWindowBackend::beginFrame(const RenderColor& clearColor) {
    if (window == nullptr) {
        return;
    }

    window->clear(sf::Color(clearColor.r, clearColor.g, clearColor.b, clearColor.a));
}

void SfmlWindowBackend::endFrame() {
    if (window != nullptr) {
        window->display();
    }
}

RenderRect SfmlWindowBackend::getViewport() const {
    if (window == nullptr) {
        return RenderRect{};
    }

    const sf::View& view = window->getView();
    const sf::Vector2f center = view.getCenter();
    const sf::Vector2f size = view.getSize();

    return RenderRect{
        center.x - size.x / 2.0f,
        center.y - size.y / 2.0f,
        size.x,
        size.y
    };
}

sf::RenderWindow& SfmlWindowBackend::getWindow() {
    return *window;
}

const sf::RenderWindow& SfmlWindowBackend::getWindow() const {
    return *window;
}
