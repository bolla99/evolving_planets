//
// Created by Giovanni Bollati on 06/03/25.
//

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>

class renderer
{
public:
    explicit renderer(SDL_Window* sdl_window);
    ~renderer();

    void update() const;

    renderer(const renderer&) = delete;
    renderer& operator=(const renderer&) = delete;
    renderer(renderer&&) = delete;
    renderer& operator=(renderer&&) = delete;

private:
    NS::SharedPtr<MTL::Device> _device;
    CA::MetalLayer* _layer;

    SDL_Renderer* sdl_renderer;
};

#endif //RENDERER_HPP
