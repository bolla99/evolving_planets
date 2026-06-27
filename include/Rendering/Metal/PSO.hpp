//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef METALPSO_HPP
#define METALPSO_HPP

#include <map>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <Rendering/IPSO.hpp>
#include <Rendering/VertexDescriptor.hpp>

namespace Rendering::Metal
{
    class PSO : public IPSO
    {
    public:
        PSO(
            const PSOConfig& config,
            MTL::Device* device,
            MTL::Library* library
            );

        // override
        [[nodiscard]] void* raw() const override { return _metalPSO.get(); }

        // observer functions
        [[nodiscard]] MTL::Function* getVertexFunction() const { return _vertexF.get(); }
        [[nodiscard]] MTL::Function* getFragmentFunction() const { return _fragmentF.get(); }

        std::map<Core::SamplerDescriptor, NS::SharedPtr<MTL::SamplerState>> samplers;


    private:
        NS::SharedPtr<MTL::RenderPipelineState> _metalPSO;
        NS::SharedPtr<MTL::Function> _vertexF;
        NS::SharedPtr<MTL::Function> _fragmentF;
    };
}

#endif //METALPSO_HPP
