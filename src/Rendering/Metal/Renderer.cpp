//
// Created by Giovanni Bollati on 06/03/25.
//

#include <simd/simd.h>
#include <Rendering/Metal/Renderer.hpp>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <MetalFX/MetalFX.hpp>
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
        
        int internalWidth = _drawableSize.x;
        int internalHeight = _drawableSize.y;

        if (useTAAScaling)
        {
            internalWidth = floor(_drawableSize.x * _TAAScaling);
            internalHeight = floor(_drawableSize.y * _TAAScaling);
        }

        static auto ringBuffer = RingBuffer(_device.get());
        ringBuffer.beginFrame();

        auto viewport = MTL::Viewport{
            viewportNormalizedRect[0] * internalWidth ,
            viewportNormalizedRect[1] * internalHeight,
            viewportNormalizedRect[2] * internalWidth,
            viewportNormalizedRect[3] * internalHeight, 0, 1};
        auto scissorRect = MTL::ScissorRect(viewport.originX, viewport.originY, viewport.width, viewport.height);

        // DEPTH STENCIL
        static auto depthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
        depthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
        depthStencilDescriptor->setDepthWriteEnabled(true);
        static auto depthStencilState = NS::TransferPtr(_device->newDepthStencilState(depthStencilDescriptor.get()));

        static auto transparentDepthStencilDescriptor = NS::TransferPtr(MTL::DepthStencilDescriptor::alloc()->init());
        transparentDepthStencilDescriptor->setDepthCompareFunction(MTL::CompareFunctionLessEqual);
        transparentDepthStencilDescriptor->setDepthWriteEnabled(false);
        static auto transparentDepthStencilState = NS::TransferPtr(_device->newDepthStencilState(transparentDepthStencilDescriptor.get()));

        static auto shadowPassDepthTextureDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        shadowPassDepthTextureDesc->setTextureType(MTL::TextureType2DArray);
        shadowPassDepthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        auto shadowPassTextureSize = 4096;
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
                    if (item.castShadows != 0)
                    {
                        bindInstanceMaterials(item.mID, shadowPSO.get(), shadowEncoder, ringBuffer);
                        shadowEncoder->setVertexBytes(&item.castShadows, sizeof(uint32_t), 26);
                        _renderables.at(item.rID)->render(&shadowRce, shadowPSO.get(), lights.value().numDirectionalLights);
                    }
                }
            }
        }
        shadowEncoder->endEncoding();

        // set global texture for shadow rendering
        static auto directionalTextureID = addTexture(shadowPassDepthTexture);
        setGlobalTexture(ShadowMap, directionalTextureID);

        // POINT LIGHT SHADOWS
        static auto pointShadowDepthTextureDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
        pointShadowDepthTextureDesc->setTextureType(MTL::TextureTypeCubeArray);
        pointShadowDepthTextureDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
        pointShadowDepthTextureDesc->setWidth(256);
        pointShadowDepthTextureDesc->setHeight(256);
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
                    if (item.castShadows != 0)
                    {
                        bindInstanceMaterials(item.mID, pointShadowPSO.get(), pointShadowEncoder, ringBuffer);
                        _renderables.at(item.rID)->render(&pointShadowRce, pointShadowPSO.get(), lights.value().numPointLights * 6);
                    }
                }
            }
        }

        pointShadowEncoder->endEncoding();

        static auto pointTextureID = addTexture(pointShadowDepthTexture);
        setGlobalTexture(CubeShadowMap, pointTextureID);


        /////////////////////// MAIN RENDER PASS ///////////////////

        // PASS DESCRIPTOR
        const auto passDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());


        // COLOR TEXTURE
        if (!_msaaTexture || _msaaTexture->width() != internalWidth || _msaaTexture->height() != internalHeight)
        {
            auto msaaDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            msaaDesc->setTextureType(MTL::TextureType2D);
            msaaDesc->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            msaaDesc->setWidth(internalWidth);
            msaaDesc->setHeight(internalHeight);
            msaaDesc->setStorageMode(MTL::StorageModePrivate);
            msaaDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            _msaaTexture = NS::TransferPtr(_device->newTexture(msaaDesc.get()));
        }

        // MOTION VECTOR TEXTURE
        static NS::SharedPtr<MTL::Texture> motionVectorTexture = nullptr;
        if (!motionVectorTexture || motionVectorTexture->width() != internalWidth || motionVectorTexture->height() != internalHeight)
        {
            std::cout << "internal resoluzione changed" << std::endl;
            auto motionVectorDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            motionVectorDesc->setTextureType(MTL::TextureType2D);
            motionVectorDesc->setPixelFormat(MTL::PixelFormatRG16Float);
            motionVectorDesc->setWidth(internalWidth);
            motionVectorDesc->setHeight(internalHeight);
            motionVectorDesc->setStorageMode(MTL::StorageModePrivate);
            motionVectorDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            motionVectorTexture = NS::TransferPtr(_device->newTexture(motionVectorDesc.get()));
        }
        
        // set color texture
        passDescriptor->colorAttachments()->object(0)->setTexture(_msaaTexture.get());
        passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 1.0});
        passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);
        // set motion vectors texture
        passDescriptor->colorAttachments()->object(1)->setTexture(motionVectorTexture.get());
        passDescriptor->colorAttachments()->object(1)->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->colorAttachments()->object(1)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 0.0});
        passDescriptor->colorAttachments()->object(1)->setStoreAction(MTL::StoreAction::StoreActionStore);


        // DEPTH TEXTURE
        static auto depthTextureID = addTexture(_depthTexture);
        if (!_depthTexture || _depthTexture->width() != internalWidth || _depthTexture->height() != internalHeight)
        {
            auto depthDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            depthDesc->setTextureType(MTL::TextureType2D);
            depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
            depthDesc->setWidth(internalWidth);
            depthDesc->setHeight(internalHeight);
            depthDesc->setStorageMode(MTL::StorageModePrivate);
            depthDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            _depthTexture = NS::TransferPtr(_device->newTexture(depthDesc.get()));
            destroyTexture(depthTextureID);
            depthTextureID = addTexture(_depthTexture);
        }

        passDescriptor->depthAttachment()->setTexture(_depthTexture.get());
        passDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
        passDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionStore);
        passDescriptor->depthAttachment()->setClearDepth(1.0);

        // main encoder
        auto encoder = buffer->renderCommandEncoder(passDescriptor.get());
        encoder->setViewport(viewport);
        encoder->setScissorRect(scissorRect);
        encoder->setDepthStencilState(depthStencilState.get());
        auto rce = CommandEncoder(encoder);

        try
        {
            encoder->setFrontFacingWinding(MTL::WindingCounterClockwise);

            for (int i = 0; i < renderQueue.queue.size(); i++)
            {
                if (i == static_cast<int>(RenderLayer::TRANSPARENT))
                    encoder->setDepthStencilState(transparentDepthStencilState.get());

                auto layer = renderQueue.queue.at(i);
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

                // after opaque materials has been rendered, we can set the depth texture as global material for transparent materials
                if (i == static_cast<int>(RenderLayer::OPAQUE))
                {
                    // set depth texture as global material
                    setGlobalTexture(OpaqueDepth, depthTextureID);
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

        encoder->endEncoding();

        
        static NS::SharedPtr<MTL::Texture> _upscaledTexture = nullptr;
        // Texture ad alta risoluzione (stessa grandezza del drawable)
        if (!_upscaledTexture || _upscaledTexture->width() != _drawableSize.x || _upscaledTexture->height() != _drawableSize.y)
        {
            auto upscaledDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            upscaledDesc->setTextureType(MTL::TextureType2D);
            upscaledDesc->setPixelFormat(_layer->pixelFormat()); // Stesso formato del drawable
            upscaledDesc->setWidth(_drawableSize.x);
            upscaledDesc->setHeight(_drawableSize.y);
            
            // IL SEGRETO È QUI:
            upscaledDesc->setStorageMode(MTL::StorageModePrivate);
            
            // ShaderRead e ShaderWrite sono richiesti da MetalFX
            upscaledDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            
            _upscaledTexture = NS::TransferPtr(_device->newTexture(upscaledDesc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _sharpenedTexture = nullptr;
        if (!_sharpenedTexture || _sharpenedTexture->width() != _drawableSize.x || _sharpenedTexture->height() != _drawableSize.y)
        {
            auto sharpenedDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            sharpenedDesc->setTextureType(MTL::TextureType2D);
            sharpenedDesc->setPixelFormat(_layer->pixelFormat());
            sharpenedDesc->setWidth(_drawableSize.x);
            sharpenedDesc->setHeight(_drawableSize.y);
            sharpenedDesc->setStorageMode(MTL::StorageModePrivate);
            sharpenedDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _sharpenedTexture = NS::TransferPtr(_device->newTexture(sharpenedDesc.get()));
        }

        // INIZIALIZZAZIONE (Una tantum)
        if (
            !_temporalScaler || _temporalScaler->outputWidth() != _drawableSize.x || _temporalScaler->outputHeight() != _drawableSize.y
            || _temporalScaler->inputWidth() != internalWidth || _temporalScaler->inputHeight() != internalHeight
            )
        {
            auto scalerDesc = NS::TransferPtr(MTLFX::TemporalScalerDescriptor::alloc()->init());
            scalerDesc->setInputWidth(internalWidth);
            scalerDesc->setInputHeight(internalHeight);
            scalerDesc->setOutputWidth(_drawableSize.x);
            scalerDesc->setOutputHeight(_drawableSize.y);
            scalerDesc->setColorTextureFormat(MTL::PixelFormatBGRA8Unorm);
            scalerDesc->setDepthTextureFormat(MTL::PixelFormatDepth32Float);
            scalerDesc->setMotionTextureFormat(MTL::PixelFormatRG16Float);
            scalerDesc->setOutputTextureFormat(MTL::PixelFormatBGRA8Unorm);
            _temporalScaler = NS::TransferPtr(scalerDesc->newTemporalScaler(_device.get()));
            _temporalScaler->setDepthReversed(false);
            _temporalScaler->setReset(true);

        }

        // -----------------------------------------------------
        // METALFX PASS
        // -----------------------------------------------------
        auto target = _msaaTexture.get();
        if (useTAAScaling)
        {
            target = _sharpenedTexture.get();
            _temporalScaler->setColorTexture(_msaaTexture.get()); // ATTENZIONE: Questo era il tuo output 3D nel codice precedente, assicurati sia la _internalColorTexture!
            _temporalScaler->setDepthTexture(_depthTexture.get());
            _temporalScaler->setMotionTexture(motionVectorTexture.get());

            _temporalScaler->setMotionVectorScaleX(internalWidth);
            _temporalScaler->setMotionVectorScaleY(internalHeight);


            // La texture di destinazione ad ALTA RISOLUZIONE
            _temporalScaler->setOutputTexture(_upscaledTexture.get());

            // Comunichiamo a MetalFX quanto abbiamo spostato la telecamera
            _temporalScaler->setJitterOffsetX(-_jitter.x);
            _temporalScaler->setJitterOffsetY(-_jitter.y);

            // Eseguiamo l'upscaling
            _temporalScaler->encodeToCommandBuffer(buffer);

            // -----------------------------------------------------
            // SHARPENING COMPUTE PASS
            // -----------------------------------------------------
            auto computeEncoder = buffer->computeCommandEncoder();
            auto sharpeningPSO = _pipelineStateObjects["Sharpening"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(sharpeningPSO->raw()));

            computeEncoder->setTexture(_upscaledTexture.get(), 0);
            computeEncoder->setTexture(_sharpenedTexture.get(), 1);

            float sharpnessVal = 0.4f; // Forza ottimale per il filtro CAS (da 0.0 a 1.0)
            computeEncoder->setBytes(&sharpnessVal, sizeof(float), 0);

            auto rawState = static_cast<MTL::ComputePipelineState*>(sharpeningPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);

            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }
        
        auto blitEncoder = buffer->blitCommandEncoder();

        auto drawable = _layer->nextDrawable();
        if (!drawable) return;
        
        _drawableSize = {
            (float)drawable->texture()->width(),
            (float)drawable->texture()->height()
        };
        
        blitEncoder->copyFromTexture(target,
                                     0,
                                     0,
                                     drawable->texture(), // Destinazione: Schermo
                                     0,
                                     0,
                                     1,
                                     1);

        blitEncoder->endEncoding();

          //////////////////////////////////////////////////
         /////////////////////// UI PASS //////////////////
        //////////////////////////////////////////////////
        // UI pass descriptor
        const auto uiPassDescriptor = NS::TransferPtr(MTL::RenderPassDescriptor::alloc()->init());

        uiPassDescriptor->colorAttachments()->object(0)->setTexture(drawable->texture()); // Disegna sullo schermo

        // ATTENZIONE: LoadActionLoad è FONDAMENTALE!
        // Dice a Metal di NON cancellare la scena 3D appena disegnata, ma di disegnarci sopra.
        uiPassDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionLoad);
        uiPassDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);

        auto uiEncoder = buffer->renderCommandEncoder(uiPassDescriptor.get());

        ImGui_ImplMetal_NewFrame(uiPassDescriptor.get());
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();

        try {
            if (_debugUICallback) {
                _debugUICallback();
            }
        } catch(std::exception& e) {
            encoder->endEncoding();
            throw std::runtime_error(e.what());
        }

        ImGui::Render();
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), buffer, uiEncoder);
        uiEncoder->endEncoding();


        buffer->presentDrawable(drawable);
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

