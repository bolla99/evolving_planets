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
        int facesCount
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
        if (faces.empty())
        {
            throw std::runtime_error("Faces data is empty");
        }
        _facesBuffer = NS::TransferPtr(rawPSO->device()->newBuffer(faces.data(), faces.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared));

        _bufferIndices = bufferIndices;

        if (_bufferIndices.size() != _buffers.size())
        {
            throw std::runtime_error("Buffer indices size does not match buffers size");
        }
        SDL_Log("Renderable successfully created with %d buffers and %d faces", static_cast<int>(_buffers.size()), static_cast<int>(faces.size()));
    }

    void Renderable::render(ICommandEncoder* commandEncoder, const glm::mat4x4& viewProjectionMatrix) const
    {
        auto metalCommandEncoder = static_cast<MTL::RenderCommandEncoder*>(commandEncoder->raw());
        if (!metalCommandEncoder)
        {
            throw std::runtime_error("Invalid command encoder type");
        }

        // bind pso
        commandEncoder->bind(_pso.get());
        // set vertex buffers
        for (int i = 0; i < _buffers.size(); i++)
        {
            // _bufferIndices contains the metal buffer index for each buffer
            static_cast<MTL::RenderCommandEncoder*>(commandEncoder->raw())->setVertexBuffer(_buffers[i].get(), 0, _bufferIndices[i]);
        }

        // Set MVP Matrix
        auto mvp = viewProjectionMatrix * _modelMatrix;
        auto mvpBuffer = NS::TransferPtr(metalCommandEncoder->device()->newBuffer(glm::value_ptr(mvp), 64, MTL::ResourceStorageModeShared));
        metalCommandEncoder->setVertexBuffer(mvpBuffer.get(), 0, 30);

        metalCommandEncoder->drawIndexedPrimitives(
            MTL::PrimitiveType::PrimitiveTypeTriangle,
            3 * _facesCount, // 3 vertices per face
            MTL::IndexType::IndexTypeUInt32,
            _facesBuffer.get(),
            0
            );
    };
}
