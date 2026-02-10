//
// Created by Giovanni Bollati on 06/03/25.
//

#ifndef APP_HPP
#define APP_HPP

#include <Rendering/Metal/Renderer.hpp>

#include "Camera.hpp"

class App
{
public:
    App(
        int width,
        int height
        );
    virtual ~App();

    App(const App& app) = delete;
    App& operator=(const App& app) = delete;

    App(App&& app) = delete;
    App& operator=(App&& app) = delete;

    virtual void init() = 0;
    virtual void run() = 0;

    static std::array<glm::vec3, 2> mouseRay(glm::mat4 viewMatrix, glm::mat4 projectionMatrix, SDL_Window* window, Rendering::IRenderer* renderer);


protected:
    SDL_Window* _window;
    std::unique_ptr<Rendering::IRenderer> _renderer;
};

#endif //APP_HPP
