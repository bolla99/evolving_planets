//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef METALCOMMANDENCODER_HPP
#define METALCOMMANDENCODER_HPP

#include <Metal/Metal.hpp>
#include <Rendering/ICommandEncoder.hpp>
#include <Rendering/IPSO.hpp>

#include "PSO.hpp"

namespace Rendering::Metal
{
    class CommandEncoder : public ICommandEncoder
    {
    public:
        CommandEncoder(MTL::RenderCommandEncoder* commandEncoder)
            : _encoder(commandEncoder) {}

        void bind(IPSO* pso) override
        {
            auto metalPSO = dynamic_cast<PSO*>(pso);
            if (!metalPSO)
            {
                throw std::runtime_error("Invalid PipelineStateObject type");
            }
            // Assuming we have a command buffer and render pass descriptor available
            _encoder->setRenderPipelineState(static_cast<MTL::RenderPipelineState*>(metalPSO->raw()));
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
