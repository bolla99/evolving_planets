//
// Created by Giovanni Bollati on 06/03/25.
//

#include "App.hpp"

#include <iostream>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <Renderer.hpp>

#include "AssimpRenderableLoader.hpp"


// app constructor initialize sdl, create a window and try to
// create a renderer from the custom class Renderer, which is in turn responsible
// for the acquisition of the sdl renderer, besides the metal structures.
App::App(
    const int width,
    const int height
    )
{
    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(SDL_GetError());
    }
    _window = SDL_CreateWindow(
        "App",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_METAL
        );
    if (!_window)
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }
    try
    {
        _renderer = std::make_unique<Renderer>(_window, std::make_unique<AssimpRenderableLoader>());
    } catch (const std::exception& e)
    {
        SDL_Quit();
        std::cerr << e.what() << std::endl;
    }
}

/*
 * sdl and window cleanup by custom sdl functions
*/
App::~App()
{
    std::cout << "app::~app()" << std::endl;
    std::cout << "Calling SDL_DestroyWindow()" << std::endl;
    SDL_DestroyWindow(_window);
    std::cout << "SDL_DestroyWindow call ended" << std::endl;
    std::cout << "Calling SDL_Quit()" << std::endl;
    SDL_Quit();
    std::cout << "SDL_Quit call ended" << std::endl;
}

// sdl loop: input management and renderer update
// right now input management is decoupled from the renderer, which means
// that the renderer is not aware of the input events, but input events can
// refer to the renderer, for example to change the state of a renderable object.
void App::run()
{
    std::cout << "app::run()" << std::endl;

    SDL_Event event;
    auto running = true;
    while (running)
    {
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
        }
        _renderer->update();
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from app::run()" << std::endl;
}
