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
#include <Rendering/PSOConfigs.hpp>
#include <Rendering/Metal/Renderable.hpp>

#include <IMeshLoader.hpp>

#include <Rendering/IRenderer.hpp>

namespace Rendering::Metal
{
    class Renderer : public IRenderer
    {
    public:
        explicit Renderer(
            SDL_Window* sdl_window,
            NS::SharedPtr<MTL::Device> device,
            NS::SharedPtr<MTL::Library> library,
            std::unique_ptr<IPSOFactory> psoFactory,
            std::unique_ptr<IRenderableFactory> renderableFactory
            );
        ~Renderer() override;

        void update(const glm::mat4x4& viewMatrix) override;

    private:
        NS::SharedPtr<MTL::Device> _device;
        CA::MetalLayer* _layer;
        NS::SharedPtr<MTL::Library> _library;

        SDL_Renderer* _sdl_renderer;
    };
}

#endif //RENDERER_HPP
