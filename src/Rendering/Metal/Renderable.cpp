//
// Created by Giovanni Bollati on 24/03/25.
//

#include <iostream>
#include <SDL_log.h>
#include <Rendering/Metal/Renderable.hpp>
#include <Rendering/Metal/PSO.hpp>
#include <Rendering/IRenderable.hpp>
#include <Metal/Metal.hpp>

#include "Rendering/Metal/Renderer.hpp"

namespace Rendering::Metal
{
    Renderable::Renderable(
        const std::vector<std::vector<float>>& data,
        const std::vector<uint32_t>& faces,
        int verticesCount,
        int facesCount,
        const std::vector<std::shared_ptr<Texture>>& textures,
        const std::vector<std::vector<std::pair<Core::VertexAttributeName, Core::VertexAttributeType>>>& vertexAttributes,
        const std::vector<Core::TextureType>& texturesTypes,
        MTL::Device* device
        ) : IRenderable(
            verticesCount,
            facesCount,
            vertexAttributes,
            texturesTypes
        )
    {
        // create buffers from data
        for (const auto& buffer : data)
        {
            auto metalBuffer = NS::TransferPtr(device->newBuffer(buffer.data(), buffer.size() * sizeof(float), MTL::ResourceStorageModeShared));
            _buffers.push_back(metalBuffer);
        }
        if (!faces.empty())
        {
            _facesBuffer = NS::TransferPtr(device->newBuffer(faces.data(), faces.size() * sizeof(uint32_t), MTL::ResourceStorageModeShared));
        }

        // set textures
        if (!textures.empty())
        {
            for (int i = 0; i < textures.size(); i++)
            {
                if (!textures[i]) continue;
                auto t = textures[i];
                auto tt = texturesTypes[i];

                auto metalTexture = Renderer::getMetalTexture(t, device);
                _textures.push_back(metalTexture);
            }
        }
    }

    void Renderable::render(
        ICommandEncoder* commandEncoder,
        IPSO* pso,
        uint instanceCount,
        DispatchSize argGridSize,
        DispatchSize argThreadgroupSize
    ) const
    {
        // return if instanceCount == 0
        if (instanceCount == 0) return;
        
        // assert command encoder and pso not null
        assert(commandEncoder && "trying to call Renderable::render with null command encoder");
        assert(pso && "trying to call Renderable::render with null pso");

        // if assert are disabled, return if nullptr
        if (!commandEncoder || !pso) return;

        // get raw command encoder
        const auto metalCommandEncoder = static_cast<MTL::RenderCommandEncoder*>(commandEncoder->raw());

        assert(metalCommandEncoder && "trying to call Renderable::render with invalid command encoder");
        if (!metalCommandEncoder) return;

        auto vertexDescriptor = pso->config.vertexDescriptor;

        // if not compatible return (only if we have attributes)
        if (!_vertexAttributes.empty() && !isCompatible(vertexDescriptor)) return;

        if (pso->config.culling == Culling::None)
        {
            metalCommandEncoder->setCullMode(MTL::CullMode::CullModeNone);
        }
        else if (pso->config.culling == Culling::Front)
        {
            metalCommandEncoder->setCullMode(MTL::CullMode::CullModeFront);
        }
        else if (pso->config.culling == Culling::Back)
        {
            metalCommandEncoder->setCullMode(MTL::CullMode::CullModeBack);
        }
        if (pso->config.pipelineType == PipelineType::Vertex)
        {
            // BIND BUFFERS
            for (int bufferIndex = 0; bufferIndex < vertexDescriptor.buffers.size(); bufferIndex++)
            {
                auto buffer = vertexDescriptor.buffers[bufferIndex];
                if (buffer.size() != 1) throw std::runtime_error("vertex descriptor with interleaved data: not supported at the moment");
                auto attribute = buffer[0];
                // search for the right buffer
                for (int i = 0; i < _vertexAttributes.size(); i++)
                {
                    if (_vertexAttributes[i].size() != 1) throw std::runtime_error("renderable with interleaved data: not supported at the moment");
                    if (attribute.name == _vertexAttributes[i][0].first and attribute.type == _vertexAttributes[i][0].second)
                    {
                        // corresponding attribute found: buffer is at index i
                        metalCommandEncoder->setVertexBuffer(_buffers[i].get(), 0, bufferIndex);
                    }
                }
            }

            // BIND SAMPLERS
            auto metalPSO = static_cast<PSO*>(pso);
            bindSamplers(metalCommandEncoder, metalPSO, pso->config.samplers);

            // BIND TEXTURES
            bindTextures(metalCommandEncoder, pso->config);

            // DRAW PRIMITIVES
            if (pso->config.primitiveType == Triangle)
            {
                metalCommandEncoder->drawIndexedPrimitives(
                    MTL::PrimitiveType::PrimitiveTypeTriangle,
                    3 * _facesCount, // 3 vertices per face
                    MTL::IndexType::IndexTypeUInt32,
                    _facesBuffer.get(),
                    0,
                    instanceCount
                    );
            }
            else if (pso->config.primitiveType == Line)
            {
                metalCommandEncoder->drawPrimitives(
                    MTL::PrimitiveTypeLine,
                    NS::UInteger(0),
                    NS::UInteger(_verticesCount),
                    NS::UInteger(1) // stride of 1 for lines
                    );
            }
        }
        else if (pso->config.pipelineType == PipelineType::Mesh)
        {
            // BIND BUFFERS
            if (!_vertexAttributes.empty())
            {
                for (int bufferIndex = 0; bufferIndex < vertexDescriptor.buffers.size(); bufferIndex++)
                {
                    auto buffer = vertexDescriptor.buffers[bufferIndex];
                    if (buffer.size() != 1) throw std::runtime_error("vertex descriptor with interleaved data: not supported at the moment");
                    auto attribute = buffer[0];

                    // search for the right buffer
                    for (int i = 0; i < _vertexAttributes.size(); i++)
                    {
                        if (_vertexAttributes[i].size() != 1) throw std::runtime_error("renderable with interleaved data: not supported at the moment");
                        if (attribute.name == _vertexAttributes[i][0].first and attribute.type == _vertexAttributes[i][0].second)
                        {
                            metalCommandEncoder->setMeshBuffer(_buffers[i].get(), 0, bufferIndex);
                            metalCommandEncoder->setObjectBuffer(_buffers[i].get(), 0, bufferIndex);
                        }
                    }
                }
            }

            // BIND INDEX BUFFER
            if (_facesBuffer && pso->config.indexBufferSlot >= 0)
            {
                metalCommandEncoder->setMeshBuffer(_facesBuffer.get(), 0, pso->config.indexBufferSlot);
                metalCommandEncoder->setObjectBuffer(_facesBuffer.get(), 0, pso->config.indexBufferSlot);
                
                // BIND INDEX COUNT (at next slot)
                uint32_t numIndices = _facesCount * 3;
                metalCommandEncoder->setMeshBytes(&numIndices, sizeof(uint32_t), pso->config.indexBufferSlot + 1);
                metalCommandEncoder->setObjectBytes(&numIndices, sizeof(uint32_t), pso->config.indexBufferSlot + 1);
            }

            // BIND SAMPLERS
            auto meshPSO = static_cast<MeshPSO*>(pso);
            bindSamplers(metalCommandEncoder, meshPSO, pso->config.samplers);

            // BIND TEXTURES
            bindTextures(metalCommandEncoder, pso->config);

            // DISPATCH MESH SHADER
            MTL::Size gridSize;
            MTL::Size threadgroupSize;

            if (argGridSize.x > 0)
            {
                gridSize = MTL::Size::Make(argGridSize.x, argGridSize.y, argGridSize.z);
                threadgroupSize = MTL::Size::Make(argThreadgroupSize.x, argThreadgroupSize.y, argThreadgroupSize.z);
            }
            else if (_gridSize.x > 0)
            {
                gridSize = MTL::Size::Make(_gridSize.x, _gridSize.y, _gridSize.z);
                threadgroupSize = MTL::Size::Make(_threadgroupSize.x, _threadgroupSize.y, _threadgroupSize.z);
            }
            else
            {
                const int trianglesPerMeshlet = 32;
                int numMeshlets = (_facesCount + trianglesPerMeshlet - 1) / trianglesPerMeshlet;
                if (numMeshlets == 0) numMeshlets = 1;

                gridSize = MTL::Size::Make(numMeshlets, 1, 1);
                threadgroupSize = MTL::Size::Make(32, 1, 1);
            }

            metalCommandEncoder->drawMeshThreadgroups(gridSize, threadgroupSize, threadgroupSize);
        }

        // reset fill mode
        metalCommandEncoder->setTriangleFillMode(MTL::TriangleFillMode::TriangleFillModeFill);
    };

    void Renderable::bindSamplers(MTL::RenderCommandEncoder* encoder, PSO* pso, const std::vector<Core::SamplerBinding>& samplers) const
    {
        for (auto samplerConfig : samplers)
        {
            auto sampler = pso->samplers.at(samplerConfig.descriptor);

            if (samplerConfig.stage == Vertex)
            {
                encoder->setVertexSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Fragment)
            {
                encoder->setFragmentSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Mesh)
            {
                encoder->setMeshSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Object)
            {
                encoder->setObjectSamplerState(sampler.get(), samplerConfig.bufferID);
            }
        }
    }

    void Renderable::bindSamplers(MTL::RenderCommandEncoder* encoder, MeshPSO* pso, const std::vector<Core::SamplerBinding>& samplers) const
    {
        for (auto samplerConfig : samplers)
        {
            auto sampler = pso->samplers.at(samplerConfig.descriptor);

            if (samplerConfig.stage == Vertex)
            {
                encoder->setVertexSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Fragment)
            {
                encoder->setFragmentSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Mesh)
            {
                encoder->setMeshSamplerState(sampler.get(), samplerConfig.bufferID);
            }
            else if (samplerConfig.stage == Object)
            {
                encoder->setObjectSamplerState(sampler.get(), samplerConfig.bufferID);
            }
        }
    }

    void Renderable::bindTextures(MTL::RenderCommandEncoder* encoder, const PSOConfig& config) const
    {
        // 1. Bind Instance Textures
        for (const auto& texturesInfo : config.instanceTextures)
        {
            bool found = false;
            // First check in _texturesTypes (from IRenderable)
            for (int j = 0; j < _texturesTypes.size(); j++)
            {
                if (_texturesTypes[j] == texturesInfo.type)
                {
                    if (j < _textures.size() && _textures[j])
                    {
                        found = true;
                        switch (texturesInfo.stage)
                        {
                            case Vertex:   encoder->setVertexTexture(_textures[j].get(), texturesInfo.bufferID);   break;
                            case Fragment: encoder->setFragmentTexture(_textures[j].get(), texturesInfo.bufferID); break;
                            case Mesh:     encoder->setMeshTexture(_textures[j].get(), texturesInfo.bufferID);     break;
                            case Object:   encoder->setObjectTexture(_textures[j].get(), texturesInfo.bufferID);   break;
                        }
                    }
                    break;
                }
            }
            
            if (!found)
            {
                // Verify if it's expected to be a global texture (optional check)
                bool isGlobal = false;
                for (const auto& globalTexture : config.globalTextures)
                {
                    if (globalTexture.type == texturesInfo.type) { isGlobal = true; break; }
                }
                
                if (!isGlobal)
                {
                    // If not found in instance textures, it might be a missing optional texture or an error
                    // std::cerr << "Renderable::bindTextures: Instance texture not found for type " << texturesInfo.type << std::endl;
                }
            }
        }
        
        // 2. Bind Global Textures (this is usually handled by the Renderer, but we check if we have any instance-specific override for global types)
        // Actually, in this architecture, global textures are handled separately in the command encoder, 
        // but we ensure that if a texture was intended as instance texture, it's bound above.
    }
}

