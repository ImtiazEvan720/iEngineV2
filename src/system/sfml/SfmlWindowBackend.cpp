#include "system/sfml/SfmlWindowBackend.h"

#include "system/IGuiBackend.h"
#include "system/InputSystem.h"
#include "system/RawInputSystem.h"

#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/Rect.hpp>
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
#include <string>

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

RawKey mapRawKey(sf::Keyboard::Key key) {
    switch (key) {
        case sf::Keyboard::Key::W:
            return RawKey::W;
        case sf::Keyboard::Key::A:
            return RawKey::A;
        case sf::Keyboard::Key::S:
            return RawKey::S;
        case sf::Keyboard::Key::D:
            return RawKey::D;
        case sf::Keyboard::Key::Space:
            return RawKey::Space;
        case sf::Keyboard::Key::Escape:
            return RawKey::Escape;
        case sf::Keyboard::Key::Backspace:
            return RawKey::Backspace;
        case sf::Keyboard::Key::Enter:
            return RawKey::Enter;
        default:
            return RawKey::Unknown;
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

RawMouseButton mapRawMouseButton(sf::Mouse::Button button) {
    switch (button) {
        case sf::Mouse::Button::Left:
            return RawMouseButton::Left;
        case sf::Mouse::Button::Right:
            return RawMouseButton::Right;
        case sf::Mouse::Button::Middle:
            return RawMouseButton::Middle;
        default:
            return RawMouseButton::Unknown;
    }
}

std::string utf8FromCodepoint(char32_t codepoint) {
    if (codepoint < 32 || codepoint == 127) {
        return "";
    }

    std::string text;
    if (codepoint <= 0x7F) {
        text.push_back(static_cast<char>(codepoint));
    } else if (codepoint <= 0x7FF) {
        text.push_back(static_cast<char>(0xC0 | ((codepoint >> 6) & 0x1F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0xFFFF) {
        text.push_back(static_cast<char>(0xE0 | ((codepoint >> 12) & 0x0F)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    } else if (codepoint <= 0x10FFFF) {
        text.push_back(static_cast<char>(0xF0 | ((codepoint >> 18) & 0x07)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 12) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F)));
        text.push_back(static_cast<char>(0x80 | (codepoint & 0x3F)));
    }

    return text;
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

    RawInputSystem& rawInputSystem = RawInputSystem::getInstance();

    while (const std::optional event = window->pollEvent()) {
        if (guiBackend != nullptr) {
            guiBackend->processNativeEvent(&*event);
        }

        if (const auto* keyPressed = event->getIf<sf::Event::KeyPressed>()) {
            inputSystem.processKeyPressed(mapKey(keyPressed->code));
            rawInputSystem.setKeyDown(mapRawKey(keyPressed->code));
        } else if (const auto* resized = event->getIf<sf::Event::Resized>()) {
            pendingResizeEvent = WindowResizeEvent{
                static_cast<int>(resized->size.x),
                static_cast<int>(resized->size.y),
                static_cast<int>(resized->size.x),
                static_cast<int>(resized->size.y)
            };
            hasPendingResizeEvent = true;
        } else if (const auto* keyReleased = event->getIf<sf::Event::KeyReleased>()) {
            inputSystem.processKeyReleased(mapKey(keyReleased->code));
            rawInputSystem.setKeyUp(mapRawKey(keyReleased->code));
        } else if (const auto* textEntered = event->getIf<sf::Event::TextEntered>()) {
            rawInputSystem.addTextInput(utf8FromCodepoint(textEntered->unicode));
        } else if (const auto* mouseMoved = event->getIf<sf::Event::MouseMoved>()) {
            rawInputSystem.setMousePosition(mouseMoved->position.x, mouseMoved->position.y);
        } else if (const auto* mousePressed = event->getIf<sf::Event::MouseButtonPressed>()) {
            inputSystem.processMousePressed(
                mapMouseButton(mousePressed->button),
                mousePressed->position.x,
                mousePressed->position.y
            );
            rawInputSystem.setMouseButtonDown(
                mapRawMouseButton(mousePressed->button),
                mousePressed->position.x,
                mousePressed->position.y
            );
        } else if (const auto* mouseReleased = event->getIf<sf::Event::MouseButtonReleased>()) {
            inputSystem.processMouseReleased(
                mapMouseButton(mouseReleased->button),
                mouseReleased->position.x,
                mouseReleased->position.y
            );
            rawInputSystem.setMouseButtonUp(
                mapRawMouseButton(mouseReleased->button),
                mouseReleased->position.x,
                mouseReleased->position.y
            );
        } else if (const auto* touchBegan = event->getIf<sf::Event::TouchBegan>()) {
            rawInputSystem.setTouchDown(
                static_cast<int>(touchBegan->finger),
                static_cast<float>(touchBegan->position.x),
                static_cast<float>(touchBegan->position.y)
            );
        } else if (const auto* touchMoved = event->getIf<sf::Event::TouchMoved>()) {
            rawInputSystem.setTouchMove(
                static_cast<int>(touchMoved->finger),
                static_cast<float>(touchMoved->position.x),
                static_cast<float>(touchMoved->position.y)
            );
        } else if (const auto* touchEnded = event->getIf<sf::Event::TouchEnded>()) {
            rawInputSystem.setTouchUp(
                static_cast<int>(touchEnded->finger),
                static_cast<float>(touchEnded->position.x),
                static_cast<float>(touchEnded->position.y)
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

    window->resetGLStates();
    const sf::Vector2u size = window->getSize();
    window->setView(sf::View(sf::FloatRect(
        {0.0f, 0.0f},
        {static_cast<float>(size.x), static_cast<float>(size.y)}
    )));
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

    const sf::Vector2u size = window->getSize();

    return RenderRect{
        0.0f,
        0.0f,
        static_cast<float>(size.x),
        static_cast<float>(size.y)
    };
}

bool SfmlWindowBackend::consumeResizeEvent(WindowResizeEvent& resizeEvent) {
    if (!hasPendingResizeEvent) {
        return false;
    }

    resizeEvent = pendingResizeEvent;
    hasPendingResizeEvent = false;
    return true;
}

sf::RenderWindow& SfmlWindowBackend::getWindow() {
    return *window;
}

const sf::RenderWindow& SfmlWindowBackend::getWindow() const {
    return *window;
}
