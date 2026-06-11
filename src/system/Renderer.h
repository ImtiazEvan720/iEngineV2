#ifndef IENGINEV2_RENDERER_H
#define IENGINEV2_RENDERER_H

namespace sf {
class RenderWindow;
}

class Level;

class Renderer {
public:
    static Renderer& getInstance();
    void setWindow(sf::RenderWindow* renderWindow);
    void render();

    Renderer(const Renderer& other) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer(Renderer&& other) = delete;
    Renderer& operator=(Renderer&& other) = delete;

private:
    Renderer() = default;
    ~Renderer() = default;

    sf::RenderWindow* window = nullptr;
};

#endif
