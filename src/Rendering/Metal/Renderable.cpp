//
// Created by Giovanni Bollati on 24/03/25.
//

#include <iostream>
#include <SDL_log.h>
#include <Rendering/Metal/Renderable.hpp>
#include <Rendering/Metal/PSO.hpp>
#include <Rendering/IRenderable.hpp>
#include <Metal/Metal.hpp>

#include "glm/gtc/type_ptr.inl"

namespace Rendering::Metal
{
    Renderable::Renderable(
        const std::vector<std::vector<float>>& data,
        const std::vector<uint32_t>& faces,
        const std::vector<int>& bufferIndices,
        const std::shared_ptr<PSO>& pso,
        int verticesCount,
        int facesCount,
        const std::vector<std::shared_ptr<Texture>>& textures
        ) : IRenderable(
            pso,
            verticesCount,
            facesCount
        )
    {
        if (data.empty())
        {
            throw std::runtime_error("Data is empty");
        }

        auto rawPSO = static_cast<MTL::RenderPipelineState*>(_pso->raw());

        // create buffers from data
        for (const auto& buffer : data)
        {
            auto metalBuffer = NS::TransferPtr(rawPSO->device()->newBuffer(buffer.data(), buffer.size() * sizeof(float), MTL::ResourceStorageModeShared));
            _buffers.push_back(metalBuffer);
        }
        if (faces.empty() && _pso->primitiveType == Triangle)
        {
            throw std::runtime_error("Faces data is empty");
        }
        if (_pso->primitiveType == Triangle)
        {
            _facesBuffer = NS::TransferPtr(rawPSO->device()->newBuffer(faces.data(), faces.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared));
        }

        _bufferIndices = bufferIndices;

        if (_bufferIndices.size() != _buffers.size())
        {
            throw std::runtime_error("Buffer indices size does not match buffers size");
        }

        // set textures
        if (!textures.empty())
        {
            std::cout << "Creating textures for renderable" << std::endl;
            for (const auto& t : textures)
            {
                auto tDesc = NS::TransferPtr(MTL::TextureDescriptor::alloc()->init());
                tDesc->setTextureType(MTL::TextureType2D);
                switch (t->format())
                {
                    case PixelFormat::R8:
                        tDesc->setPixelFormat(MTL::PixelFormatR8Unorm);
                        break;
                    case PixelFormat::RGB8:
                        throw std::runtime_error("RGB8 format is not supported in Metal, use RGBA8 instead");
                    case PixelFormat::RGBA8:
                        tDesc->setPixelFormat(MTL::PixelFormatRGBA8Unorm);
                        break;
                    default:
                        throw std::runtime_error("Unsupported texture format");
                }
                tDesc->setWidth(t->width());
                tDesc->setHeight(t->height());
                tDesc->setStorageMode(MTL::StorageModeShared);
                tDesc->setUsage(MTL::TextureUsageShaderRead);

                auto metalTexture = NS::TransferPtr(rawPSO->device()->newTexture(tDesc.get()));
                metalTexture->replaceRegion(
                                            MTL::Region::Make2D(0, 0, t->width(), t->height()),
                                            0,
                                            t->getData().data(),
                                            t->bytesPerPixel() * t->width()
                                            );
                _textures.push_back(metalTexture);
            }
        }
        //SDL_Log("Renderable successfully created with %d buffers and %d faces", static_cast<int>(_buffers.size()), static_cast<int>(faces.size()));
    }

    void Renderable::render(ICommandEncoder* commandEncoder, const glm::mat4x4& viewProjectionMatrix) const
    {
        auto metalCommandEncoder = static_cast<MTL::RenderCommandEncoder*>(commandEncoder->raw());
        if (!metalCommandEncoder)
        {
            throw std::runtime_error("Invalid command encoder type");
        }

        // bind pso ( skip because it binds in the renderer loop)
        //commandEncoder->bind(_pso.get());
        // set vertex buffers
        for (int i = 0; i < _buffers.size(); i++)
        {
            // _bufferIndices contains the metal buffer index for each buffer
            static_cast<MTL::RenderCommandEncoder*>(commandEncoder->raw())->setVertexBuffer(_buffers[i].get(), 0, _bufferIndices[i]);
        }

        for (int i = 0; i < _textures.size(); i++)
        {
            //SDL_Log("Setting Texture");
            // set textures
            metalCommandEncoder->setFragmentTexture(_textures[i].get(), i);
        }

        // Set MVP Matrix
        auto mvp = viewProjectionMatrix * _modelMatrix;
        auto mvpBuffer = NS::TransferPtr(metalCommandEncoder->device()->newBuffer(glm::value_ptr(_modelMatrix), 64, MTL::ResourceStorageModeShared));
        metalCommandEncoder->setVertexBuffer(mvpBuffer.get(), 0, 29);

        // set materials
        for (int i = 0; i < _materials.size(); i++)
        {
            const auto& material = _materials[i];
            auto info = _materialInfos[i];
            auto data = material.data();
            auto materialBuffer = NS::TransferPtr(metalCommandEncoder->device()->newBuffer(data, material.size(), MTL::ResourceStorageModeShared));
            if (info.stage == MaterialStage::Vertex)
            {
                metalCommandEncoder->setVertexBuffer(materialBuffer.get(), 0, info.bufferIndex);
            }
            else
            {
                metalCommandEncoder->setFragmentBuffer(materialBuffer.get(), 0, info.bufferIndex);
            }
        }

        // set fill mode
        if (wireframe)
        {
            metalCommandEncoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeLines);
        }
        else
        {
            metalCommandEncoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeFill);
        }

        if (_pso->primitiveType == Triangle)
        {
            metalCommandEncoder->drawIndexedPrimitives(
                MTL::PrimitiveType::PrimitiveTypeTriangle,
                3 * _facesCount, // 3 vertices per face
                MTL::IndexType::IndexTypeUInt32,
                _facesBuffer.get(),
                0
                );
        } else if (_pso->primitiveType == Line)
        {
            metalCommandEncoder->drawPrimitives(
                MTL::PrimitiveTypeLine,
                NS::UInteger(0),
                NS::UInteger(_verticesCount),
                NS::UInteger(1) // stride of 1 for lines
                );
        }
        // reset fill mode
        metalCommandEncoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeFill);
    };
}

