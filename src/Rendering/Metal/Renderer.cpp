//
// Created by Giovanni Bollati on 06/03/25.
//

#include <simd/simd.h>
#include <../../include/Rendering/Metal/Renderer.hpp>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>
#include <ranges>
#include <set>

#include <Rendering/Metal/CommandEncoder.hpp>
#include <Rendering/Metal/PSOFactory.hpp>
#include <Rendering/Metal/Renderable.hpp>
#include <Rendering/Metal/RenderableFactory.hpp>
#include <Rendering/Metal/Util/Utils.hpp>

#include "imgui_impl_metal.h"
#include "imgui_impl_sdl2.h"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"

// resources acquisition
// device, layer, library, PSOs, VertexDescriptors
namespace Rendering::Metal
{
    Renderer::Renderer(
        SDL_Window* sdl_window,
        NS::SharedPtr<MTL::Device> device,
        NS::SharedPtr<MTL::Library> library,
        std::unique_ptr<IPSOFactory> psoFactory,
        std::unique_ptr<IRenderableFactory> renderableFactory
        ) : IRenderer(
            std::move(psoFactory),
            std::move(renderableFactory)
            )
    {
        if (!device)
        {
            throw std::runtime_error("Metal device passed to renderer constructor is null");
        }
        _device = device;

        // INIT IMGUI METAL IMPL
        ImGui_ImplMetal_Init(_device.get());

        if (!library)
        {
            throw std::runtime_error("Metal library passed to renderer constructor is null");
        }
        _library = library;

        _sdl_renderer = SDL_CreateRenderer(sdl_window, -1, SDL_RENDERER_PRESENTVSYNC);
        if (!_sdl_renderer)
        {
            throw std::runtime_error(SDL_GetError());
        }

        _layer = static_cast<CA::MetalLayer*>(SDL_RenderGetMetalLayer(_sdl_renderer));
        if (!_layer) throw std::runtime_error(SDL_GetError());

        _layer->setDevice(_device.get());
        _layer->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
        
        auto drawableSize = _layer->drawableSize();
        SDL_Log("DRAWABLE SIZE: %f x %f", drawableSize.width, drawableSize.height);
        loadPSOs(psoConfigs);
    }

    // rendering loop
    void Renderer::update(const glm::mat4x4& viewMatrix)
    {
        auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
        const auto queue = NS::TransferPtr(_device->newCommandQueue());
        const auto drawable = _layer->nextDrawable();

        const auto passDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        passDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture());
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.5, 0.0, 0.5, 1.0});
        passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);

        // depth texture
        // 1. Crea il descriptor
        auto depthDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        depthDesc->setTextureType(MTL::TextureType2D);
        depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        depthDesc->setWidth(drawable->texture()->width());
        depthDesc->setHeight(drawable->texture()->height());
        depthDesc->setStorageMode(MTL::StorageModePrivate);
        depthDesc->setUsage(MTL::TextureUsageRenderTarget);

        // 2. Crea la texture
        auto depthTexture = NS::TransferPtr(_device->newTexture(depthDesc.get()));

        passDescriptor->depthAttachment()->setTexture(depthTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionStore);
        passDescriptor->depthAttachment()->setClearDepth(1.0);

        const auto buffer = queue->commandBuffer();

        // encoder ownership is never obtained
        // an eventual abstract wrapper should not take ownership of the encoder
        const auto encoder = buffer->renderCommandEncoder(passDescriptor.get());

        ImGui_ImplMetal_NewFrame(passDescriptor.get());
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        // set depth test
        auto depthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLess);
        depthStencilDescriptor->setDepthWriteEnabled(true);
        auto depthStencilState = NS::TransferPtr(_device->newDepthStencilState(depthStencilDescriptor.get()));

        encoder->setDepthStencilState(depthStencilState.get());

        // set sampler
        auto samplerDescriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
        samplerDescriptor->setMinFilter(MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear);
        samplerDescriptor->setMagFilter(MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear);
        samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
        samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
        samplerDescriptor->setNormalizedCoordinates(true);
        auto sampler = NS::TransferPtr(encoder->device()->newSamplerState(samplerDescriptor.get()));
        encoder->setFragmentSamplerState(sampler.get(), 0);
        

        // set lights
        auto lightsBuffer = NS::TransferPtr(_device->newBuffer(
            &_lights,
            sizeof(_lights),
            MTL::ResourceStorageModeShared
        ));
        encoder->setVertexBuffer(
            lightsBuffer.get(),
            0,
            28 // buffer index 1
        );

        // SET DRAWABLE SIZE BUFFER
        float drawableSize[2] = {
            (float)drawable->texture()->width(),
            (float)drawable->texture()->height()
        };
        auto drawableSizeBuffer = NS::TransferPtr(_device->newBuffer(
            &drawableSize,
            2 * sizeof(float),
            MTL::ResourceStorageModeShared
        ));
        encoder->setVertexBuffer(
            drawableSizeBuffer.get(),
            0,
            20 // buffer index 2
        );


        const auto viewProjectionMatrix = glm::perspective(
            glm::radians(55.0f),
            (float)drawable->texture()->width() / (float)drawable->texture()->height(),
            0.1f,
            1000.0f) * viewMatrix;

        encoder->setVertexBytes(
            &viewProjectionMatrix,
            sizeof(viewProjectionMatrix),
            30 // vertex buffer index 0
        );

        auto rce = CommandEncoder(encoder);

        // render the renderables
        //std::cout << "Rendering " << _renderables.size() << " renderables." << std::endl;
        for (const auto& renderable : _renderables[static_cast<int>(RenderLayer::BACKGROUND)] | std::views::values)
        {
            renderable->render(&rce, viewProjectionMatrix);
        }
        for (const auto& renderable : _renderables[static_cast<int>(RenderLayer::OPAQUE)] | std::views::values)
        {
            renderable->render(&rce, viewProjectionMatrix);
        }
        for (const auto& renderable : _renderables[static_cast<int>(RenderLayer::TRANSPARENT)] | std::views::values)
        {
            renderable->render(&rce, viewProjectionMatrix);
        }
        for (const auto& renderable : _renderables[static_cast<int>(RenderLayer::UI)] | std::views::values)
        {
            renderable->render(&rce, viewProjectionMatrix);
        }
        for (const auto& renderable : _renderables[static_cast<int>(RenderLayer::TEXT)] | std::views::values)
        {
            renderable->render(&rce, viewProjectionMatrix);
        }


        _debugUICallback();

        ImGui::Begin("Hello Cane");
        ImGui::Text("CANE PELOSO");

        ImGui::End();
        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, encoder);


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
}

