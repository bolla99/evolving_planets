//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef METALCOMMANDENCODER_HPP
#define METALCOMMANDENCODER_HPP

#include <Metal/Metal.hpp>
#include <Rendering/ICommandEncoder.hpp>
#include <Rendering/IPSO.hpp>

#include "PSO.hpp"
#include "MeshPSO.hpp"

namespace Rendering::Metal
{
    class CommandEncoder : public ICommandEncoder
    {
    public:
        CommandEncoder(MTL::RenderCommandEncoder* commandEncoder)
            : _encoder(commandEncoder) {}

        void bind(IPSO* pso) override
        {
            _encoder->setRenderPipelineState(static_cast<MTL::RenderPipelineState*>(pso->raw()));
            /*
            if (pso->config.pipelineType == PipelineType::Vertex)
            {
                auto metalPSO = dynamic_cast<PSO*>(pso);
                if (!metalPSO)
                {
                    throw std::runtime_error("Invalid PipelineStateObject type: expected PSO for Vertex pipeline");
                }
                _encoder->setRenderPipelineState(static_cast<MTL::RenderPipelineState*>(metalPSO->raw()));
            }
            else if (pso->config.pipelineType == PipelineType::Mesh)
            {
                auto metalMeshPSO = dynamic_cast<MeshPSO*>(pso);
                if (!metalMeshPSO)
                {
                    throw std::runtime_error("Invalid PipelineStateObject type: expected MeshPSO for Mesh pipeline");
                }
                _encoder->setRenderPipelineState(static_cast<MTL::RenderPipelineState*>(metalMeshPSO->raw()));
            }
            else
            {
                throw std::runtime_error("Unknown pipeline type");
            }
            */
        }

        void* raw() override
        {
            return _encoder;
        }

    private:
        MTL::RenderCommandEncoder* _encoder; // MTL::CommandEncoder pointer
    };
}

#endif //METALCOMMANDENCODER_HPP
