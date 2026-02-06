//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef METALPSO_HPP
#define METALPSO_HPP

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
            MTL::Library* library,
            MTL::PixelFormat pixelFormat
            );

        // override
        [[nodiscard]] void* raw() const override { return _metalPSO.get(); }

        // observer functions
        [[nodiscard]] MTL::Function* getVertexFunction() const { return _vertexF.get(); }
        [[nodiscard]] MTL::Function* getFragmentFunction() const { return _fragmentF.get(); }

    private:
        NS::SharedPtr<MTL::RenderPipelineState> _metalPSO;
        NS::SharedPtr<MTL::Function> _vertexF;
        NS::SharedPtr<MTL::Function> _fragmentF;
    };
}

#endif //METALPSO_HPP
