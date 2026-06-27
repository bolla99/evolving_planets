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

#include "RingBuffer.hpp"
#include "../../Engine/ECS/Pool.hpp"
#include "MetalFX/MTLFXTemporalScaler.hpp"

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

        void update(const RenderQueue& renderQueue, const glm::vec4& viewportNormalizedRect) override;

        void compute(
            const std::string& psoName,
            const std::vector<std::shared_ptr<Core::Texture>>& inputTextures,
            const std::vector<std::shared_ptr<Core::Texture>>& outputTextures,
            const std::vector<std::pair<MaterialType, std::vector<std::byte>>>& materials,
            glm::ivec3 grid,
            glm::ivec3 threadgroup
        ) override;

        uint64_t addGlobalTexture(const std::shared_ptr<Texture>& texture) override
        {
            auto metalTexture = Renderer::getMetalTexture(texture, _device.get());
            return addTexture(metalTexture);
        }

        void destroyGlobalTexture(uint64_t id) override
        {
            if (_textures.contains(id)) _texturePool.destroyID(id);
        }

        static NS::SharedPtr<MTL::Texture> getMetalTexture(const std::shared_ptr<Texture> tex, MTL::Device* device)
        {
            if (!tex->is3D())
            {
                auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                desc->setWidth(tex->width());
                desc->setHeight(tex->height());

                if (tex->format() == Core::RGBA32F) {
                    desc->setPixelFormat(MTL::PixelFormatRGBA32Float);
                } else if (tex->format() == Core::R32F) {
                    desc->setPixelFormat(MTL::PixelFormatR32Float);
                } else {
                    desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
                }

                desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
                desc->setStorageMode(MTL::StorageModeShared);

                auto mtlTex = NS::TransferPtr(device->newTexture(desc.get()));

                if (!tex->getData().empty()) {
                    size_t bytesPerRow = tex->width() * tex->bytesPerPixel();
                    mtlTex->replaceRegion(MTL::Region(0, 0, tex->width(), tex->height()), 0, tex->getData().data(), bytesPerRow);
                }
                return mtlTex;
            }
            else
            {
                auto desc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                desc->setWidth(tex->width());
                desc->setHeight(tex->height());
                desc->setDepth(tex->depth());
                desc->setTextureType(MTL::TextureType3D);

                if (tex->format() == Core::RGBA32F) {
                    desc->setPixelFormat(MTL::PixelFormatRGBA32Float);
                } else if (tex->format() == Core::R32F) {
                    desc->setPixelFormat(MTL::PixelFormatR32Float);
                } else {
                    desc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
                }

                desc->setUsage(MTL::TextureUsageShaderRead | MTL::TextureUsageShaderWrite | MTL::TextureUsageRenderTarget);
                desc->setStorageMode(MTL::StorageModeShared);

                auto mtlTex = NS::TransferPtr(device->newTexture(desc.get()));

                if (!tex->getData().empty()) {
                    size_t bytesPerRow = tex->width() * tex->bytesPerPixel();
                    size_t bytesPerImage = tex->height() * bytesPerRow;
                    auto region = MTL::Region::Make3D(0, 0, 0, tex->width(), tex->height(), tex->depth());
                    mtlTex->replaceRegion(region, 0, 0, tex->getData().data(), bytesPerRow, bytesPerImage);
                }
                return mtlTex;
            }
        }

    private:
        NS::SharedPtr<MTL::Device> _device;
        CA::MetalLayer* _layer;
        NS::SharedPtr<MTL::Library> _library;
        CA::MetalDrawable* _drawable;

        SDL_Renderer* _sdl_renderer;

        // GLOBAL TEXTURES
        std::unordered_map<uint64_t, NS::SharedPtr<MTL::Texture>> _textures;
        Pool _texturePool;

        // HELPER METHODS

        void bindGlobalMaterials(IPSO* pso, MTL::RenderCommandEncoder* encoder, RingBuffer& ringBuffer)
        {
            // SET PSO MATERIALS
            for (int j = 0; j < pso->config.globalMaterials.size(); j++)
            {
                const auto& materialInfo = pso->config.globalMaterials[j];
                auto& material = _globalMaterials.at(pso->config.name).at(materialInfo.type);
                size_t offset;
                if (material.writeCounter < 3)
                {
                    // must be written
                    offset = ringBuffer.write(material.bytes);
                    material.cachedOffsets[ringBuffer.currentFrame] = offset;
                    material.writeCounter++;
                } else
                {
                    offset = material.cachedOffsets[ringBuffer.currentFrame];
                    ringBuffer.advance(material.bytes.size());
                }

                switch (materialInfo.stage)
                {
                    case MaterialStage::Vertex:
                        encoder->setVertexBuffer(ringBuffer.buffer.get(), offset, materialInfo.bufferIndex);
                        break;
                    case MaterialStage::Fragment:
                        encoder->setFragmentBuffer(ringBuffer.buffer.get(), offset, materialInfo.bufferIndex);
                        break;
                    case MaterialStage::Mesh:
                        encoder->setMeshBuffer(ringBuffer.buffer.get(), offset, materialInfo.bufferIndex);
                        break;
                    case MaterialStage::Object:
                        encoder->setObjectBuffer(ringBuffer.buffer.get(), offset, materialInfo.bufferIndex);
                        break;
                }
            }
        }

        void bindInstanceMaterials(uint64_t mID, IPSO* pso, MTL::RenderCommandEncoder* encoder, RingBuffer& ringBuffer)
        {
            // set instance material
            if (!_instanceMaterials.contains(mID))
            {
                std::cerr << "instance material not found" << std::endl;
                throw std::runtime_error("instance material not found");
            }
            auto& materials = _instanceMaterials.at(mID);
            auto infos = pso->config.instanceMaterials;
            for (auto info : infos)
            {
                if (!materials.contains(info.type)) continue;
                auto& material = materials.at(info.type);
                size_t offset;
                if (material.writeCounter < 3)
                {
                    // must be written
                    offset = ringBuffer.write(material.bytes);
                    material.cachedOffsets[ringBuffer.currentFrame] = offset;
                    material.writeCounter++;
                } else
                {
                    offset = material.cachedOffsets[ringBuffer.currentFrame];
                    ringBuffer.advance(material.bytes.size());
                }
                
                switch (info.stage)
                {
                    case MaterialStage::Vertex:
                        encoder->setVertexBuffer(ringBuffer.buffer.get(), offset, info.bufferIndex);
                        break;
                    case MaterialStage::Fragment:
                        encoder->setFragmentBuffer(ringBuffer.buffer.get(), offset, info.bufferIndex);
                        break;
                    case MaterialStage::Mesh:
                        encoder->setMeshBuffer(ringBuffer.buffer.get(), offset, info.bufferIndex);
                        break;
                    case MaterialStage::Object:
                        encoder->setObjectBuffer(ringBuffer.buffer.get(), offset, info.bufferIndex);
                        break;
                }
            }
        }

        [[nodiscard]] uint64_t addTexture(const NS::SharedPtr<MTL::Texture>& t)
        {
            uint64_t newID = _texturePool.newID();
            _textures.insert({newID, t});
            return newID;
        }

        void destroyTexture(const uint64_t id)
        {
            if (_textures.contains(id))
            {
                _textures.erase(id);
                _texturePool.destroyID(id);
            }
        }

        void bindGlobalTextures(const std::string& pso, MTL::RenderCommandEncoder* encoder) const
        {
            if (!_pipelineStateObjects.contains(pso))
            {
                //std::cout << "trying to bind global textures for the pso " << pso << " but this pso does not exists" << std::endl;
                return;
            }
            auto config = _pipelineStateObjects.at(pso)->config;
            for (const auto& texture : config.globalTextures)
            {
                if (!_globalTextures.contains(pso))
                {
                    //std::cout << "the pso: " << pso << " requires global textures but no global texture has been set for this pso yet" << std::endl;
                    break;
                }
                if (_globalTextures.at(pso).contains(texture.type))
                {
                    auto textureID = _globalTextures.at(pso).at(texture.type);
                    if (_textures.contains(textureID))
                    {
                        if (texture.stage == MaterialStage::Vertex)
                            encoder->setVertexTexture(_textures.at(textureID).get(), texture.bufferID);
                        else if (texture.stage == MaterialStage::Fragment)
                            encoder->setFragmentTexture(_textures.at(textureID).get(), texture.bufferID);
                        else if (texture.stage == MaterialStage::Mesh)
                            encoder->setMeshTexture(_textures.at(textureID).get(), texture.bufferID);
                        else if (texture.stage == MaterialStage::Object)
                            encoder->setObjectTexture(_textures.at(textureID).get(), texture.bufferID);
                    }
                }
                else
                {
                    //std::cout << "the pso: " << pso << " requires a global texture that does not exists yet" << std::endl;
                }
            }
        }

        NS::SharedPtr<MTL::Texture> _msaaTexture;
        NS::SharedPtr<MTL::Texture> _depthTexture;
        NS::SharedPtr<MTLFX::TemporalScaler> _temporalScaler;
    };
}

#endif //RENDERER_HPP
