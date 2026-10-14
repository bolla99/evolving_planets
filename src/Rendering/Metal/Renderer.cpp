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

        if (useScaling)
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

        /*
        if (shadowPSO->config.culling == Front)
            shadowEncoder->setCullMode(MTL::CullModeFront);
        else
            shadowEncoder->setCullMode(MTL::CullModeBack);
            */
        if (useDirectionalShadowsFrontCulling)
        {
            shadowEncoder->setCullMode(MTL::CullModeFront);
        } else
        {
            shadowEncoder->setCullMode(MTL::CullModeBack);
        }

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

        /*
        if (pointShadowPSO->config.culling == Front)
            pointShadowEncoder->setCullMode(MTL::CullModeFront);
        else
            pointShadowEncoder->setCullMode(MTL::CullModeBack);
            */

        if (usePointShadowsFrontCulling)
        {
            pointShadowEncoder->setCullMode(MTL::CullModeFront);
        }
        else
        {
            pointShadowEncoder->setCullMode(MTL::CullModeBack);
        }
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
        if (!_nonMSAATexture || _nonMSAATexture->width() != internalWidth || _nonMSAATexture->height() != internalHeight)
        {
            auto msaaDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            msaaDesc->setTextureType(MTL::TextureType2D);
            msaaDesc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            msaaDesc->setWidth(internalWidth);
            msaaDesc->setHeight(internalHeight);
            msaaDesc->setStorageMode(MTL::StorageModePrivate);
            msaaDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            _nonMSAATexture = NS::TransferPtr(_device->newTexture(msaaDesc.get()));
        }

        // COLOR TEXTURE
        if (!_msaaTexture || _msaaTexture->width() != internalWidth || _msaaTexture->height() != internalHeight)
        {
            auto msaaDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            msaaDesc->setTextureType(MTL::TextureType2DMultisample);
            msaaDesc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            msaaDesc->setWidth(internalWidth);
            msaaDesc->setHeight(internalHeight);
            msaaDesc->setSampleCount(4);
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

        // MOTION VECTOR TEXTURE
        static NS::SharedPtr<MTL::Texture> msaaMotionVectorTexture = nullptr;
        if (!msaaMotionVectorTexture || msaaMotionVectorTexture->width() != internalWidth || msaaMotionVectorTexture->height() != internalHeight)
        {
            std::cout << "internal resoluzione changed" << std::endl;
            auto motionVectorDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            motionVectorDesc->setTextureType(MTL::TextureType2DMultisample);
            motionVectorDesc->setPixelFormat(MTL::PixelFormatRG16Float);
            motionVectorDesc->setWidth(internalWidth);
            motionVectorDesc->setHeight(internalHeight);
            motionVectorDesc->setSampleCount(4);
            motionVectorDesc->setStorageMode(MTL::StorageModePrivate);
            motionVectorDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            msaaMotionVectorTexture = NS::TransferPtr(_device->newTexture(motionVectorDesc.get()));
        }
        
        // set color texture
        if (!useMSAA)
        {
            passDescriptor->colorAttachments()->object(0)->setTexture(_nonMSAATexture.get());
            passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
            passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 1.0});
            passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreAction::StoreActionStore);
        } else
        {
            passDescriptor->colorAttachments()->object(0)->setTexture(_msaaTexture.get());
            passDescriptor->colorAttachments()->object(0)->setLoadAction(MTL::LoadAction::LoadActionClear);
            passDescriptor->colorAttachments()->object(0)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 1.0});
            passDescriptor->colorAttachments()->object(0)->setStoreAction(MTL::StoreActionMultisampleResolve);
            passDescriptor->colorAttachments()->object(0)->setResolveTexture(_nonMSAATexture.get());
        }
        // set motion vectors texture
        if (!useMSAA)
        {
            passDescriptor->colorAttachments()->object(1)->setTexture(motionVectorTexture.get());
            passDescriptor->colorAttachments()->object(1)->setLoadAction(MTL::LoadAction::LoadActionClear);
            passDescriptor->colorAttachments()->object(1)->setClearColor(MTL::ClearColor{0.0f, 0.0f, 0.0f, 0.0});
            passDescriptor->colorAttachments()->object(1)->setStoreAction(MTL::StoreAction::StoreActionStore);
        }
        else
        {
            passDescriptor->colorAttachments()->object(1)->setTexture(msaaMotionVectorTexture.get());
            passDescriptor->colorAttachments()->object(1)->setStoreAction(MTL::StoreAction::StoreActionDontCare);
        }


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

        // DEPTH TEXTURE
        if (!_msaaDepthTexture || _msaaDepthTexture->width() != internalWidth || _msaaDepthTexture->height() != internalHeight)
        {
            auto depthDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            depthDesc->setTextureType(MTL::TextureType2DMultisample);
            depthDesc->setSampleCount(4);
            depthDesc->setPixelFormat(MTL::PixelFormatDepth32Float);
            depthDesc->setWidth(internalWidth);
            depthDesc->setHeight(internalHeight);
            depthDesc->setStorageMode(MTL::StorageModePrivate);
            depthDesc->setUsage(MTL::TextureUsageRenderTarget | MTL::TextureUsageShaderRead);
            _msaaDepthTexture = NS::TransferPtr(_device->newTexture(depthDesc.get()));
        }

        if (!useMSAA)
        {
            passDescriptor->depthAttachment()->setTexture(_depthTexture.get());
            passDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
            passDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionStore);
            passDescriptor->depthAttachment()->setClearDepth(1.0);
        }
        else
        {
            passDescriptor->depthAttachment()->setTexture(_msaaDepthTexture.get());
            passDescriptor->depthAttachment()->setLoadAction(MTL::LoadAction::LoadActionClear);
            passDescriptor->depthAttachment()->setStoreAction(MTL::StoreAction::StoreActionMultisampleResolve);
            passDescriptor->depthAttachment()->setResolveTexture(_depthTexture.get());
        }

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

                    // set correct msaa usage; now raw will return the correct pso
                    pso->usingMSAA = useMSAA;
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
                    pso->usingMSAA = useMSAA;

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
            upscaledDesc->setPixelFormat(MTL::PixelFormatRGBA16Float); // HDR format for upscale target
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

        // --- ALLOCATE HDR BLOOM TEXTURES AND COMPOSITE TEXTURES ---
        int bloomWidth = _drawableSize.x / 2;
        int bloomHeight = _drawableSize.y / 2;
        if (bloomWidth < 1) bloomWidth = 1;
        if (bloomHeight < 1) bloomHeight = 1;

        static NS::SharedPtr<MTL::Texture> _bloomTextureDown = nullptr;
        if (!_bloomTextureDown || _bloomTextureDown->width() != bloomWidth || _bloomTextureDown->height() != bloomHeight)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(bloomWidth);
            desc->setHeight(bloomHeight);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _bloomTextureDown = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _bloomTextureTemp = nullptr;
        if (!_bloomTextureTemp || _bloomTextureTemp->width() != bloomWidth || _bloomTextureTemp->height() != bloomHeight)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(bloomWidth);
            desc->setHeight(bloomHeight);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _bloomTextureTemp = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _bloomTextureFinal = nullptr;
        if (!_bloomTextureFinal || _bloomTextureFinal->width() != bloomWidth || _bloomTextureFinal->height() != bloomHeight)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(bloomWidth);
            desc->setHeight(bloomHeight);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _bloomTextureFinal = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _combinedHDRTexture = nullptr;
        if (!_combinedHDRTexture || _combinedHDRTexture->width() != _drawableSize.x || _combinedHDRTexture->height() != _drawableSize.y)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(_drawableSize.x);
            desc->setHeight(_drawableSize.y);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _combinedHDRTexture = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _tonemappedTexture = nullptr;
        if (!_tonemappedTexture || _tonemappedTexture->width() != _drawableSize.x || _tonemappedTexture->height() != _drawableSize.y)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(_layer->pixelFormat());
            desc->setWidth(_drawableSize.x);
            desc->setHeight(_drawableSize.y);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _tonemappedTexture = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        // TEMPORAL SCALER
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
            scalerDesc->setColorTextureFormat(MTL::PixelFormatRGBA16Float); // Input is HDR
            scalerDesc->setDepthTextureFormat(MTL::PixelFormatDepth32Float);
            scalerDesc->setMotionTextureFormat(MTL::PixelFormatRG16Float);
            scalerDesc->setOutputTextureFormat(MTL::PixelFormatRGBA16Float); // Output is HDR
            _temporalScaler = NS::TransferPtr(scalerDesc->newTemporalScaler(_device.get()));
            _temporalScaler->setDepthReversed(false);
            _temporalScaler->setReset(true);
        }

        // SPATIAL SCALER
        if (
            !_spatialScaler || _spatialScaler->outputWidth() != _drawableSize.x || _spatialScaler->outputHeight() != _drawableSize.y
            || _spatialScaler->inputWidth() != internalWidth || _spatialScaler->inputHeight() != internalHeight
            )
        {
            auto scalerDesc = NS::TransferPtr(MTLFX::SpatialScalerDescriptor::alloc()->init());
            scalerDesc->setInputWidth(internalWidth);
            scalerDesc->setInputHeight(internalHeight);
            scalerDesc->setOutputWidth(_drawableSize.x);
            scalerDesc->setOutputHeight(_drawableSize.y);
            scalerDesc->setColorTextureFormat(MTL::PixelFormatRGBA16Float); // Input is HDR
            scalerDesc->setOutputTextureFormat(MTL::PixelFormatRGBA16Float); // Output is HDR
            _spatialScaler = NS::TransferPtr(scalerDesc->newSpatialScaler(_device.get()));
        }

        // -----------------------------------------------------
        // METALFX PASS (IF ENABLED)
        // -----------------------------------------------------
        if (useTAAScaling)
        {
            _temporalScaler->setColorTexture(_nonMSAATexture.get());
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
        }
        else if (useScaling)
        {
            _spatialScaler->setColorTexture(_nonMSAATexture.get());

            // L'output è la texture nativa della finestra/schermo (es. 4K)
            _spatialScaler->setOutputTexture(_upscaledTexture.get());

            // 3. Inietta i comandi di scaling nel command buffer
            _spatialScaler->encodeToCommandBuffer(buffer);
        }

        // -----------------------------------------------------
        // COMPUTE-BASED POST-PROCESSING PIPELINE
        // -----------------------------------------------------
        MTL::Texture* hdrSceneTexture = useScaling ? _upscaledTexture.get() : _nonMSAATexture.get();

        // Pass 1: Bloom Threshold & Downsample (e.g. from full res to 1/2 res)
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto bloomThresholdPSO = _pipelineStateObjects["BloomThreshold"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(bloomThresholdPSO->raw()));
            computeEncoder->setTexture(hdrSceneTexture, 0);
            computeEncoder->setTexture(_bloomTextureDown.get(), 1);
            computeEncoder->setBytes(&bloomThreshold, sizeof(float), 0);

            auto rawState = static_cast<MTL::ComputePipelineState*>(bloomThresholdPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(bloomWidth, bloomHeight, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // Pass 2: Bloom Blur Horizontal
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto bloomBlurHPSO = _pipelineStateObjects["BloomBlurH"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(bloomBlurHPSO->raw()));
            computeEncoder->setTexture(_bloomTextureDown.get(), 0);
            computeEncoder->setTexture(_bloomTextureTemp.get(), 1);

            auto rawState = static_cast<MTL::ComputePipelineState*>(bloomBlurHPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(bloomWidth, bloomHeight, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // Pass 3: Bloom Blur Vertical
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto bloomBlurVPSO = _pipelineStateObjects["BloomBlurV"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(bloomBlurVPSO->raw()));
            computeEncoder->setTexture(_bloomTextureTemp.get(), 0);
            computeEncoder->setTexture(_bloomTextureFinal.get(), 1);

            auto rawState = static_cast<MTL::ComputePipelineState*>(bloomBlurVPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(bloomWidth, bloomHeight, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // Pass 4: Bloom Combine (hdrSceneTexture + bloom -> _combinedHDRTexture)
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto bloomCombinePSO = _pipelineStateObjects["BloomCombine"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(bloomCombinePSO->raw()));
            computeEncoder->setTexture(hdrSceneTexture, 0);
            computeEncoder->setTexture(_bloomTextureFinal.get(), 1);
            computeEncoder->setTexture(_combinedHDRTexture.get(), 2);
            computeEncoder->setBytes(&bloomIntensity, sizeof(float), 0);

            auto rawState = static_cast<MTL::ComputePipelineState*>(bloomCombinePSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // Pass 5: Tonemapping (_combinedHDRTexture -> _tonemappedTexture LDR)
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto tonemappingPSO = _pipelineStateObjects["Tonemapping"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(tonemappingPSO->raw()));
            computeEncoder->setTexture(_combinedHDRTexture.get(), 0);
            computeEncoder->setTexture(_tonemappedTexture.get(), 1);

            auto rawState = static_cast<MTL::ComputePipelineState*>(tonemappingPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // SMAA ANTIALIASING PASS
        // Alloca le texture necessarie per SMAA
        static NS::SharedPtr<MTL::Texture> _smaaEdgeTexture = nullptr;
        if (!_smaaEdgeTexture || _smaaEdgeTexture->width() != _drawableSize.x || _smaaEdgeTexture->height() != _drawableSize.y)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(_drawableSize.x);
            desc->setHeight(_drawableSize.y);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _smaaEdgeTexture = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _smaaBlendTexture = nullptr;
        if (!_smaaBlendTexture || _smaaBlendTexture->width() != _drawableSize.x || _smaaBlendTexture->height() != _drawableSize.y)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(_drawableSize.x);
            desc->setHeight(_drawableSize.y);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _smaaBlendTexture = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        static NS::SharedPtr<MTL::Texture> _smaaOutputTexture = nullptr;
        if (!_smaaOutputTexture || _smaaOutputTexture->width() != _drawableSize.x || _smaaOutputTexture->height() != _drawableSize.y)
        {
            auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
            desc->setTextureType(MTL::TextureType2D);
            desc->setPixelFormat(MTL::PixelFormatRGBA16Float);
            desc->setWidth(_drawableSize.x);
            desc->setHeight(_drawableSize.y);
            desc->setStorageMode(MTL::StorageModePrivate);
            desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite);
            _smaaOutputTexture = NS::TransferPtr(_device->newTexture(desc.get()));
        }

        if (useSMAA)
        // Pass 6a: SMAA Edge Detection (_tonemappedTexture -> _smaaEdgeTexture)
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto edgeDetectionPSO = _pipelineStateObjects["SMAAEdgeDetection"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(edgeDetectionPSO->raw()));
            computeEncoder->setTexture(_tonemappedTexture.get(), 0);
            computeEncoder->setTexture(_smaaEdgeTexture.get(), 1);
            computeEncoder->setBytes(&smaaThreshold, sizeof(float), 0);

            auto rawState = static_cast<MTL::ComputePipelineState*>(edgeDetectionPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();

        // Pass 6b: SMAA Blend Weight Calculation (_smaaEdgeTexture -> _smaaBlendTexture)
            computeEncoder = buffer->computeCommandEncoder();
            auto blendWeightPSO = _pipelineStateObjects["SMAABlendWeight"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(blendWeightPSO->raw()));
            computeEncoder->setTexture(_smaaEdgeTexture.get(), 0);
            computeEncoder->setTexture(_smaaBlendTexture.get(), 1);
            computeEncoder->setBytes(&smaaIntensity, sizeof(float), 0);

            rawState = static_cast<MTL::ComputePipelineState*>(blendWeightPSO->raw());
            maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            widthThreads = rawState->threadExecutionWidth();
            heightThreads = maxThreads / widthThreads;

            threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();

        // Pass 6c: SMAA Neighborhood Blending (_tonemappedTexture + _smaaBlendTexture -> _smaaOutputTexture)
            computeEncoder = buffer->computeCommandEncoder();
            auto neighborhoodBlendingPSO = _pipelineStateObjects["SMAANeighborhoodBlending"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(neighborhoodBlendingPSO->raw()));
            computeEncoder->setTexture(_tonemappedTexture.get(), 0);
            computeEncoder->setTexture(_smaaBlendTexture.get(), 1);
            computeEncoder->setTexture(_smaaOutputTexture.get(), 2);
            computeEncoder->setBytes(&smaaIntensity, sizeof(float), 0);

            rawState = static_cast<MTL::ComputePipelineState*>(neighborhoodBlendingPSO->raw());
            maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            widthThreads = rawState->threadExecutionWidth();
            heightThreads = maxThreads / widthThreads;

            threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);
            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        // Aggiorna il target per lo sharpening (ora la input è l'output di SMAA anziché tonemapping)
        MTL::Texture* sharpeningInput = _tonemappedTexture.get();
        if (useSMAA)
        {
            sharpeningInput = _smaaOutputTexture.get();
        }

        // Pass 6: Sharpening (sharpeningInput -> _sharpenedTexture CAS)
        {
            auto computeEncoder = buffer->computeCommandEncoder();
            auto sharpeningPSO = _pipelineStateObjects["Sharpening"];
            computeEncoder->setComputePipelineState(static_cast<MTL::ComputePipelineState*>(sharpeningPSO->raw()));
            computeEncoder->setTexture(sharpeningInput, 0);
            computeEncoder->setTexture(_sharpenedTexture.get(), 1);

            computeEncoder->setBytes(&sharpening, sizeof(float), 0);

            auto rawState = static_cast<MTL::ComputePipelineState*>(sharpeningPSO->raw());
            auto maxThreads = rawState->maxTotalThreadsPerThreadgroup();
            auto widthThreads = rawState->threadExecutionWidth();
            auto heightThreads = maxThreads / widthThreads;

            MTL::Size threadgroupSize = MTL::Size(widthThreads, heightThreads, 1);
            MTL::Size gridSize = MTL::Size(_drawableSize.x, _drawableSize.y, 1);

            computeEncoder->dispatchThreads(gridSize, threadgroupSize);
            computeEncoder->endEncoding();
        }

        auto target = _sharpenedTexture.get();

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

