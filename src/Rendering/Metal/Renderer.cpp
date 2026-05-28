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
#include <vector>
#include <unordered_map>

#include <Rendering/Metal/CommandEncoder.hpp>
#include <Rendering/Metal/PSOFactory.hpp>
#include <Rendering/Metal/Renderable.hpp>
#include <Rendering/Metal/RenderableFactory.hpp>
#include <Rendering/Metal/Util/Utils.hpp>

#include "imgui_impl_metal.h"
#include "imgui_impl_sdl2.h"
#include "timer.hpp"
#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "Rendering/Metal/RingBuffer.hpp"

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
    void Renderer::update(const RenderQueue& renderQueue, const glm::vec4& viewportNormalizedRect)
    {
        auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
        static const auto queue = NS::TransferPtr(_device->newCommandQueue());
        const auto buffer = queue->commandBuffer();

        _drawable = _layer->nextDrawable();

        _drawableSize = {
            (float)_drawable->texture()->width(),
            (float)_drawable->texture()->height()
        };

        static auto ringBuffer = RingBuffer(_device.get());
        ringBuffer.beginFrame();

        auto viewport = MTL::Viewport{viewportNormalizedRect[0] * _drawableSize[0], viewportNormalizedRect[1] * _drawableSize[1], viewportNormalizedRect[2] * (_drawableSize[0]), viewportNormalizedRect[3] * _drawableSize[1], 0, 1};
        auto scissorRect = MTL::ScissorRect(viewport.originX, viewport.originY, viewport.width, viewport.height);

        // DEPTH STENCIL
        static auto depthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
        depthStencilDescriptor->setDepthWriteEnabled(true);
        static auto depthStencilState = NS::TransferPtr(_device->newDepthStencilState(depthStencilDescriptor.get()));

        static auto shadowPassDepthTextureDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        shadowPassDepthTextureDesc->setTextureType(MTL::TextureType2DArray);
        shadowPassDepthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        auto shadowPassTextureSize = 1024;
        shadowPassDepthTextureDesc->setWidth(shadowPassTextureSize);
        shadowPassDepthTextureDesc->setHeight(shadowPassTextureSize);
        shadowPassDepthTextureDesc->setArrayLength(MAX_DIRECTIONAL_LIGHTS);
        shadowPassDepthTextureDesc->setStorageMode(MTL::StorageModePrivate);
        shadowPassDepthTextureDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
        static auto shadowPassDepthTexture = NS::TransferPtr(_device->newTexture(shadowPassDepthTextureDesc.get()));

        // DIRECTIONAL SHADOWS
        auto lights = getGlobalMaterial<Lights>(LIGHTS);
        static const auto shadowPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        shadowPassDescriptor->depthAttachment()->setTexture(shadowPassDepthTexture.get());
        shadowPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
        shadowPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionStore);
        shadowPassDescriptor->depthAttachment()->setClearDepth(1.0);
        shadowPassDescriptor->colorAttachments()->object(0)->setTexture(nullptr);

        shadowPassDescriptor->setRenderTargetArrayLength(shadowPassDepthTexture->arrayLength());

        auto shadowEncoder = buffer->renderCommandEncoder(shadowPassDescriptor.get());
        shadowEncoder->setDepthStencilState(depthStencilState.get());

        auto shadowRce = CommandEncoder(shadowEncoder);
        auto shadowPSO = _pipelineStateObjects["DirectionalShadow"];
        shadowRce.bind(shadowPSO.get());

        shadowEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        if (shadowPSO->config.culling == Front)
            shadowEncoder->setCullMode(MTL::CullModeFront);
        else
            shadowEncoder->setCullMode(MTL::CullModeBack);

        bindGlobalMaterials(shadowPSO.get(), shadowEncoder, ringBuffer);

        for (auto layer : renderQueue.queue)
        {
            for (auto& [psoName, items] : layer)
            {
                for (const auto& item : items)
                {
                    if (item.castShadows)
                    {
                        bindInstanceMaterials(item.mID, shadowPSO.get(), shadowEncoder, ringBuffer);
                        _renderables.at(item.rID)->render(&shadowRce, shadowPSO.get(), lights.value().numDirectionalLights);
                    }
                }
            }
        }
        shadowEncoder->endEncoding();

        // set global texture for shadow rendering
        auto textureID = addTexture(shadowPassDepthTexture);
        setGlobalTexture(ShadowMap, textureID);

        // POINT LIGHT SHADOWS
        static auto pointShadowDepthTextureDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        pointShadowDepthTextureDesc->setTextureType(MTL::TextureTypeCubeArray);
        pointShadowDepthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        pointShadowDepthTextureDesc->setWidth(128);
        pointShadowDepthTextureDesc->setHeight(128);
        pointShadowDepthTextureDesc->setArrayLength(MAX_POINT_LIGHTS);
        pointShadowDepthTextureDesc->setStorageMode(MTL::StorageModePrivate);
        pointShadowDepthTextureDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
        static auto pointShadowDepthTexture = NS::TransferPtr(_device->newTexture(pointShadowDepthTextureDesc.get()));

        auto pointShadowPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        pointShadowPassDescriptor->depthAttachment()->setTexture(pointShadowDepthTexture.get());
        pointShadowPassDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
        pointShadowPassDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionStore);
        pointShadowPassDescriptor->depthAttachment()->setClearDepth(1.0);
        pointShadowPassDescriptor->colorAttachments()->object(0)->setTexture(nullptr);
        pointShadowPassDescriptor->setRenderTargetArrayLength(pointShadowDepthTexture->arrayLength() * 6);
        auto pointShadowEncoder = buffer->renderCommandEncoder(pointShadowPassDescriptor.get());
        pointShadowEncoder->setDepthStencilState(depthStencilState.get());
        auto pointShadowRce = CommandEncoder(pointShadowEncoder);
        auto pointShadowPSO = _pipelineStateObjects["PointShadow"];
        pointShadowRce.bind(pointShadowPSO.get());
        bindGlobalMaterials(pointShadowPSO.get(), pointShadowEncoder, ringBuffer);

        pointShadowEncoder->setFrontFacingWinding(MTL::WindingCounterClockwise);
        if (pointShadowPSO->config.culling == Front)
            pointShadowEncoder->setCullMode(MTL::CullModeFront);
        else
            pointShadowEncoder->setCullMode(MTL::CullModeBack);
        for (auto layer : renderQueue.queue)
        {
            for (auto& [psoName, items] : layer)
            {
                for (const auto& item : items)
                {
                    if (item.castShadows)
                    {
                        bindInstanceMaterials(item.mID, pointShadowPSO.get(), pointShadowEncoder, ringBuffer);
                        _renderables.at(item.rID)->render(&pointShadowRce, pointShadowPSO.get(), lights.value().numPointLights * 6);
                    }
                }
            }
        }

        pointShadowEncoder->endEncoding();

        auto id = addTexture(pointShadowDepthTexture);
        setGlobalTexture(CubeShadowMap, id);


        /////////////////////// MAIN RENDER PASS ///////////////////

        // PASS DESCRIPTOR
        const auto passDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

        int sampleCount = 4; // Default to 4
        // check if at least one pso in the queue has msaa enabled
        // for now we force it globally if any pso uses it, but since we set it in PSOConfig it should be consistent
        // Actually, let's just use 4 as requested/defaulted.

        // MSAA TEXTURE
        if (!_msaaTexture || _msaaTexture->width() != _drawable->texture()->width() || _msaaTexture->height() != _drawable->texture()->height())
        {
            if (sampleCount > 1)
            {
                auto msaaDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                msaaDesc->setTextureType(MTL::TextureType2DMultisample);
                msaaDesc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
                msaaDesc->setWidth(_drawable->texture()->width());
                msaaDesc->setHeight(_drawable->texture()->height());
                msaaDesc->setSampleCount(sampleCount);
                msaaDesc->setStorageMode(MTL::StorageModePrivate);
                msaaDesc->setUsage(MTL::TextureUsageRenderTarget);
                _msaaTexture = NS::TransferPtr(_device->newTexture(msaaDesc.get()));
            } else {
                auto msaaDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                msaaDesc->setTextureType(MTL::TextureType2D);
                msaaDesc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
                msaaDesc->setWidth(_drawable->texture()->width());
                msaaDesc->setHeight(_drawable->texture()->height());
                msaaDesc->setStorageMode(MTL::StorageModePrivate);
                msaaDesc->setUsage(MTL::TextureUsageRenderTarget);
                _msaaTexture = NS::TransferPtr(_device->newTexture(msaaDesc.get()));
            }
        }

        if (sampleCount > 1) {
            passDescriptor->colorAttachments()->object(0)->setTexture(_msaaTexture.get());
            passDescriptor->colorAttachments()->object(0)->setResolveTexture(_drawable->texture());
        } else {
            passDescriptor->colorAttachments()->object(0)->setTexture(_drawable->texture());
        }
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 1.0});
        if (sampleCount > 1) {
            passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionMultisampleResolve);
        } else {
            passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);
        }

        // DEPTH TEXTURE
        if (!_depthTexture || _depthTexture->width() != _drawable->texture()->width() || _depthTexture->height() != _drawable->texture()->height())
        {
            if (sampleCount > 1)
            {
                auto depthDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                depthDesc->setTextureType(MTL::TextureType2DMultisample);
                depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
                depthDesc->setWidth(_drawable->texture()->width());
                depthDesc->setHeight(_drawable->texture()->height());
                depthDesc->setSampleCount(sampleCount);
                depthDesc->setStorageMode(MTL::StorageModePrivate);
                depthDesc->setUsage(MTL::TextureUsageRenderTarget);
                _depthTexture = NS::TransferPtr(_device->newTexture(depthDesc.get()));
            } else {
                auto depthDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                depthDesc->setTextureType(MTL::TextureType2D);
                depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
                depthDesc->setWidth(_drawable->texture()->width());
                depthDesc->setHeight(_drawable->texture()->height());
                depthDesc->setStorageMode(MTL::StorageModePrivate);
                depthDesc->setUsage(MTL::TextureUsageRenderTarget);
                _depthTexture = NS::TransferPtr(_device->newTexture(depthDesc.get()));
            }
        }

        passDescriptor->depthAttachment()->setTexture(_depthTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionDontCare);
        passDescriptor->depthAttachment()->setClearDepth(1.0);

        // main encoder
        auto encoder = buffer->renderCommandEncoder(passDescriptor.get());
        encoder->setViewport(viewport);
        encoder->setScissorRect(scissorRect);
        encoder->setDepthStencilState(depthStencilState.get());
        auto rce = CommandEncoder(encoder);

        ImGui_ImplMetal_NewFrame(passDescriptor.get());
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        try
        {
            encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

            for (auto layer : renderQueue.queue)
            {
                for (auto& [psoName, items] : layer)
                {
                    auto pso = _pipelineStateObjects.at(psoName);
                    rce.bind(pso.get());

                    if (pso->config.culling == Front)
                        encoder->setCullMode(MTL::CullModeFront);
                    else
                        encoder->setCullMode(MTL::CullModeBack);

                    bindGlobalMaterials(pso.get(), encoder, ringBuffer);
                    bindGlobalTextures(psoName, encoder);

                    for (const auto & item : items)
                    {
                        bindInstanceMaterials(item.mID, pso.get(), encoder, ringBuffer);

                        if (item.wireframe)
                            encoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeLines);
                        else
                            encoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeFill);

                        _renderables.at(item.rID)->render(
                            &rce, 
                            pso.get(), 
                            1, 
                            {item.gridSize.x, item.gridSize.y, item.gridSize.z}, 
                            {item.threadgroupSize.x, item.threadgroupSize.y, item.threadgroupSize.z}
                        );
                    }
                }
            }

            // IMMEDIATE
            for (auto layer : _immediateRenderQueue.queue)
            {
                if (layer.empty()) continue;
                for (auto& [psoName, items] : layer)
                {
                    auto pso = _pipelineStateObjects.at(psoName);

                    rce.bind(pso.get());

                    if (pso->config.culling == Front)
                        encoder->setCullMode(MTL::CullModeFront);
                    else
                        encoder->setCullMode(MTL::CullModeBack);

                    bindGlobalMaterials(pso.get(), encoder, ringBuffer);
                    bindGlobalTextures(psoName, encoder);

                    for (const auto & item : items)
                    {
                        bindInstanceMaterials(item.mID, pso.get(), encoder, ringBuffer);

                        if (item.wireframe)
                            encoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeLines);
                        else
                            encoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeFill);

                        _renderables.at(item.rID)->render(
                            &rce, 
                            pso.get(), 
                            1, 
                            {item.gridSize.x, item.gridSize.y, item.gridSize.z}, 
                            {item.threadgroupSize.x, item.threadgroupSize.y, item.threadgroupSize.z}
                        );
                    }
                }
            }
        }
        catch (std::exception& e)
        {
            encoder->endEncoding();
            std::cerr << e.what() << std::endl;
            throw std::runtime_error(e.what());
        }

        try {
            if (_debugUICallback) {
                _debugUICallback();
            }
        } catch(std::exception& e) {
            encoder->endEncoding();
            throw std::runtime_error(e.what());
        }

        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, encoder);

        encoder->endEncoding();


        /*
        // Depth visualization pass
        auto secondPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());
        secondPassDescriptor->colorAttachments()->object(0)->setTexture(_drawable->texture());
        secondPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionLoad);
        secondPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);
        auto secondEncoder = buffer->renderCommandEncoder(secondPassDescriptor.get());

        // DRAW DEPTH MAP
        try
        {
            // draw depth map
            auto mesh = Mesh::quad({0.0f, 0.0f, 1, 1}, 0.1, {0.0f, 0.0f, 0.0f, 1.0f}, 1.0, 1.0);
            auto renderable = addRenderable(*mesh, {}, true);
            auto material = addDefaultInstanceMaterial("Depth", true);
            static_cast<Renderable*>(_renderables.at(renderable).get())->setMetalTexture({shadowPassDepthTexture}, {ShadowMap});
            setInstanceMaterial(material, getBytes(glm::vec4{0.0f, 0.0f, 200.0f, 200.0f}), MaterialType::RECT);

            // set global materials
            auto pso = _pipelineStateObjects.at("Depth");
            bindGlobalMaterials(pso.get(), secondEncoder, ringBuffer);
            bindInstanceMaterials(material, pso.get(), secondEncoder, ringBuffer);
            auto r = _renderables.at(renderable);
            auto secondRce = CommandEncoder(secondEncoder);
            secondRce.bind(pso.get());
            _renderables.at(renderable)->render(&secondRce, pso.get());
        }
        catch (std::exception& e)
        {
            secondEncoder->endEncoding();
            std::cerr << e.what() << std::endl;
            throw std::runtime_error(e.what());
        }

        secondEncoder->endEncoding();
        */


        buffer->presentDrawable(_drawable);
        buffer->commit();

        // clear immediate data
        for (auto& id : _immediateInstanceMaterials)
        {
            removeInstanceMaterial(id);
        }
        for (auto& id : _immediateRenderables)
        {
            removeRenderable(id);
        }
        _immediateInstanceMaterials.clear();
        _immediateRenderables.clear();
        _immediateRenderQueue.clear();
    }

    void Renderer::compute(
        const std::string& psoName,
        const std::vector<std::shared_ptr<Core::Texture>>& inputTextures,
        const std::vector<std::shared_ptr<Core::Texture>>& outputTextures,
        const std::vector<std::pair<MaterialType, std::vector<std::byte>>>& materials,
        glm::ivec3 grid, glm::ivec3 threadgroup)
    {
        if (!_pipelineStateObjects.contains(psoName))
        {
            throw std::runtime_error("PSO not found: " + psoName);
        }
        auto pso = _pipelineStateObjects.at(psoName);
        auto computePSO = dynamic_cast<ComputePSO*>(pso.get());
        if (!computePSO)
        {
            throw std::runtime_error("PSO is not a ComputePSO: " + psoName);
        }

        auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
        static const auto queue = NS::TransferPtr(_device->newCommandQueue());
        const auto buffer = queue->commandBuffer();

        const auto encoder = buffer->computeCommandEncoder();
        encoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(computePSO->raw()));

        // Bind materials
        static auto ringBuffer = RingBuffer(_device.get());
        ringBuffer.beginFrame();
        
        for (auto mat : computePSO->config.instanceMaterials) {
            auto type = mat.type;
            for (int i = 0; i < materials.size(); i++) {
                if (materials[i].first == type) {
                    auto offset = ringBuffer.write(materials[i].second);
                    encoder->setBuffer(ringBuffer.buffer.get(), offset, mat.bufferIndex);
                }
            }
        }
        /*
        for (int i = 0; i < materials.size(); ++i)
        {
            auto offset = ringBuffer.write(materials[i].second);
            encoder->setBuffer(ringBuffer.buffer.get(), offset, i);
        }*/

        // Helper to convert/get MTL::Texture
        auto getMTLTexture = [&](const std::shared_ptr<Core::Texture>& tex) -> MTL::Texture* {
            // Search in _textures if we have an ID for this texture
            // For simplicity in this compute call, we create/update a managed texture.
            
            MTL::TextureDescriptor* desc = MTL::TextureDescriptor::alloc()->init();
            desc->setWidth(tex->width());
            desc->setHeight(tex->height());
            
            // Handle different pixel formats
            if (tex->format() == Core::RGBA32F) {
                desc->setPixelFormat(MTL::PixelFormatRGBA32Float);
            } else if (tex->format() == Core::R32F) {
                desc->setPixelFormat(MTL::PixelFormatR32Float);
            } else {
                desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
            }
            
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
            desc->setStorageMode(MTL::StorageModeShared);
            
            auto mtlTex = _device->newTexture(desc);
            desc->release();
            
            if (!tex->getData().empty()) {
                size_t bytesPerRow = tex->width() * tex->bytesPerPixel();
                mtlTex->replaceRegion(MTL::Region(0, 0, tex->width(), tex->height()), 0, tex->getData().data(), bytesPerRow);
            }
            return mtlTex;
        };

        for (int i = 0; i < inputTextures.size(); ++i)
        {
            auto mtlTex = getMTLTexture(inputTextures[i]);
            encoder->setTexture(mtlTex, i);
            mtlTex->release(); // If created new
        }
        
        std::vector<MTL::Texture*> mtlOutputTextures;
        for (int i = 0; i < outputTextures.size(); ++i)
        {
            auto mtlTex = getMTLTexture(outputTextures[i]);
            encoder->setTexture(mtlTex, i + inputTextures.size());
            mtlOutputTextures.push_back(mtlTex);
        }

        // Metal recommends calculating the grid size based on texture dimensions
        // and using a threadgroup size that is a multiple of execution width (32 for Metal)
        MTL::Size gridSize = MTL::Size(grid.x, grid.y, grid.z);
        MTL::Size threadgroupSize = MTL::Size(threadgroup.x, threadgroup.y, threadgroup.z);
        
        // Use dispatchThreads for simpler automatic grid management
        encoder->dispatchThreads(gridSize, threadgroupSize);

        encoder->endEncoding();
        buffer->commit();
        buffer->waitUntilCompleted();
        
        // Copy back results to Core::Texture
        for (int i = 0; i < outputTextures.size(); ++i) {
            auto tex = outputTextures[i];
            auto mtlTex = mtlOutputTextures[i];
            size_t bytesPerRow = tex->width() * tex->bytesPerPixel();
            
            // Ensure data vector is correctly sized
            tex->updateData(std::vector<uint8_t>(tex->height() * bytesPerRow));
            mtlTex->getBytes(tex->getData().data(), bytesPerRow, MTL::Region(0, 0, tex->width(), tex->height()), 0);

            // Debug: check if data is all zeros
            if (tex->format() == Core::RGBA32F || tex->format() == Core::R32F) {
                const float* floatData = (const float*)tex->getData().data();
                bool allZeros = true;
                size_t numFloats = tex->getData().size() / sizeof(float);
                for(size_t j=0; j < numFloats; ++j) {
                    if(floatData[j] != 0.0f) {
                        allZeros = false;
                        break;
                    }
                }
                if(allZeros) {
                    std::cout << "WARNING: Texture " << tex->type() << " is ALL ZEROS after compute!" << std::endl;
                } else {
                    std::cout << "Texture " << tex->type() << " generated successfully (non-zero)." << std::endl;
                }
            }
            mtlTex->release();
        }
    }

    Renderer::~Renderer()
    {
        std::cout << "renderer::~renderer()" << std::endl;
        std::cout << "Calling SDL_DestroyRenderer" << std::endl;
        SDL_DestroyRenderer(_sdl_renderer);
        std::cout << "SDL_DestroyRenderer call ended" << std::endl;
    }
}

