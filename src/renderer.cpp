//
// Created by Giovanni Bollati on 06/03/25.
//


#include "renderer.hpp"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>

renderer::renderer(SDL_Window* sdl_window)
{
    _device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!_device) throw std::runtime_error("Failed to create device");

    sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!sdl_renderer)
    {
        throw std::runtime_error(SDL_GetError());
    }

    _layer = static_cast<CA::MetalLayer*>(SDL_RenderGetMetalLayer(sdl_renderer));
    if (!_layer)
    {
        throw std::runtime_error(SDL_GetError());
    }
    _layer->setDevice(_device.get());
    _layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
}

void renderer::update() const
{
    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    const auto queue = NS::TransferPtr(_device->newCommandQueue());
    const auto drawable = _layer->nextDrawable();

    const auto passDescriptor =NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    passDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
    passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
    passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.5, 0.0, 0.5, 1.0});
    passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);

    const auto buffer = queue->commandBuffer();

    const auto encoder = buffer->renderCommandEncoder(passDescriptor.get());

    encoder->endEncoding();

    buffer->presentDrawable(drawable);
    buffer->commit();
}

renderer::~renderer()
{
    std::cout << "renderer::~renderer()" << std::endl;
    std::cout << "Calling SDL_DestroyRenderer" << std::endl;
    
    SDL_DestroyRenderer(sdl_renderer);
}

