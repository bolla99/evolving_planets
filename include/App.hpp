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

    virtual App& init();
    virtual App& run();

private:
    SDL_Window* _window;
    std::unique_ptr<Rendering::IRenderer> _renderer;
    std::unique_ptr<Camera> _camera;
};

#endif //APP_HPP
