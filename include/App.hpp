//
// Created by Giovanni Bollati on 06/03/25.
//

#ifndef APP_HPP
#define APP_HPP
#include <memory>
#include <Renderer.hpp>

class App
{
public:
    App(
        int width,
        int height
        );
    ~App();

    App(const App& app) = delete;
    App& operator=(const App& app) = delete;

    App(App&& app) = delete;
    App& operator=(App&& app) = delete;

    void virtual run();

private:
    SDL_Window* _window;
    std::unique_ptr<Renderer> _renderer;
};

#endif //APP_HPP
