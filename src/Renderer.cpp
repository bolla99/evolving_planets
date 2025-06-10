//
// Created by Giovanni Bollati on 06/03/25.
//

#include <simd/simd.h>
#include "Renderer.hpp"
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>
#include <set>

#include "AssimpRenderableLoader.hpp"
#include "Renderable.hpp"

using MTL::PrimitiveType;

// resources acquisition
// device, layer, library, PSOs, VertexDescriptors
Renderer::Renderer(SDL_Window* sdl_window, std::unique_ptr<IRenderableLoader> renderableLoader) : _renderableLoader(std::move(renderableLoader))
{
    _device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!_device) throw std::runtime_error("Failed to create device");

    _sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
    if (!_sdl_renderer)
    {
        throw std::runtime_error(SDL_GetError());
    }

    _layer = static_cast<CA::MetalLayer*>(SDL_RenderGetMetalLayer(_sdl_renderer));
    if (!_layer) throw std::runtime_error(SDL_GetError());

    _layer->setDevice(_device.get());
    _layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

    auto libraryPath = NS::Bundle::mainBundle()->resourcePath()->stringByAppendingString(
    NS::String::string("/Shaders.metallib", NS::ASCIIStringEncoding)
        );
    std::cout << libraryPath->utf8String() << std::endl;
    NS::Error* error;
    _library = NS::TransferPtr(_device->newLibrary(libraryPath, &error));
    if (!_library.get()) throw std::runtime_error(error->localizedDescription()->utf8String());

    _pipelineStateObjects = std::unordered_map<std::string, std::shared_ptr<PipelineStateObject>>();

    loadPSOs(psoConfigs);

    // try loading an assimp asset
    auto monkey = _renderableLoader->loadRenderable(
        NS::Bundle::mainBundle()->resourcePath()->stringByAppendingString(NS::String::string("/monkey.obj", NS::ASCIIStringEncoding))->utf8String(),
        _pipelineStateObjects.at("PC"),
        _device.get()
        );
}

// rendering loop
void Renderer::update() const
{
    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    const auto queue = NS::TransferPtr(_device->newCommandQueue());
    const auto drawable = _layer->nextDrawable();
    
    simd::float3 triangle[] = {
        {0.5f, 0.0f, 0.0f},
        {-0.5f, 0.5f, 0.0f},
        {-0.5f, -0.5f, 0.0f}
    };
    simd::float4 triangle_colors[] = {
        {1.0f, 0.0f, 0.0f, 1.0f},
        {0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, 0.0f, 1.0f, 1.0f}
    };
    auto triangle_renderable = Renderable(
        {{static_cast<int>(sizeof(triangle)), triangle}, {static_cast<int>(sizeof(triangle_colors)), triangle_colors}},
        _pipelineStateObjects.at("PC"),
        3,
        _device.get()
        );

    const auto passDescriptor =NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
    passDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
    passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
    passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.5, 0.0, 0.5, 1.0});
    passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);

    const auto buffer = queue->commandBuffer();

    const auto encoder = buffer->renderCommandEncoder(passDescriptor.get());

    auto positions_buffer =NS::TransferPtr( _device->newBuffer(triangle, sizeof(triangle), MTL::ResourceStorageModeShared));
    auto colors_buffer = NS::TransferPtr(_device->newBuffer(triangle_colors, sizeof(triangle_colors), MTL::ResourceStorageModeShared));

    /*
    encoder->setRenderPipelineState(_PSOs.at("PC")->get_pso());
    encoder->setVertexBuffer(positions_buffer.get(), 0, 0);
    encoder->setVertexBuffer(colors_buffer.get(), 0, 1);
    auto mvp = simd::float4x4(1.0f);
    auto mvp_buffer = NS::TransferPtr(_device->newBuffer(&mvp, sizeof(mvp), MTL::ResourceStorageModeShared));
    encoder->setVertexBuffer(mvp_buffer.get(), 0, 30);

    encoder->drawPrimitives(
        MTL::PrimitiveType::PrimitiveTypeTriangle,
        NS::UInteger(0),
        NS::UInteger(3));
    */
    triangle_renderable.render(encoder, simd::float4x4(1.0f));

    // render all the renderable
    for (const auto renderable : _renderables)
    {
        renderable->render(encoder, simd::float4x4(1.0f));
    }

    encoder->endEncoding();

    buffer->presentDrawable(drawable);
    buffer->commit();
}

Renderer::~Renderer()
{
    std::cout << "renderer::~renderer()" << std::endl;
    std::cout << "Calling SDL_DestroyRenderer" << std::endl;
    SDL_DestroyRenderer(_sdl_renderer);
    std::cout << "SDL_DestroyRenderer call ended" << std::endl;
}

void Renderer::loadPSOs(const std::vector<PSOConfig>& psoConfigs)
{
    for (const auto& config : psoConfigs)
    {
        createPSO(config);
    }
}

void Renderer::createPSO(const PSOConfig& config)
{
    try
    {
        _pipelineStateObjects.emplace(
            config.name,
            std::make_shared<PipelineStateObject>(
                config.name, config.vertexShader, config.fragmentShader, _device.get(), _library.get(), _layer->pixelFormat(), config.vertexDescriptor)
                );
    } catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
}


