//
// Created by Giovanni Bollati on 06/03/25.
//

#ifndef APP_HPP
#define APP_HPP
#include <memory>
#include <renderer.hpp>

class app
{
public:
    app(
        int width,
        int height
        );
    ~app();

    app(const app& app) = delete;
    app& operator=(const app& app) = delete;

    app(app&& app) = delete;
    app& operator=(app&& app) = delete;

    void virtual run();

private:
    SDL_Window* _window;
    std::unique_ptr<renderer> _renderer;
};

#endif //APP_HPP
