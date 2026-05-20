//
// Created by Giovanni Bollati on 21/04/26.
//

#ifndef MESHPSO_HPP
#define MESHPSO_HPP

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <Rendering/IPSO.hpp>
#include <Rendering/VertexDescriptor.hpp>

namespace Rendering::Metal
{
    class MeshPSO : public IPSO
    {
    public:
        MeshPSO(
            const PSOConfig& config,
            MTL::Device* device,
            MTL::Library* library
        );

        // override
        [[nodiscard]] void* raw() const override { return _metalPSO.get(); }

        // observer functions
        [[nodiscard]] MTL::Function* getObjectFunction() const { return _objectF.get(); }
        [[nodiscard]] MTL::Function* getMeshFunction() const { return _meshF.get(); }
        [[nodiscard]] MTL::Function* getFragmentFunction() const { return _fragmentF.get(); }

    private:
        NS::SharedPtr<MTL::RenderPipelineState> _metalPSO;
        NS::SharedPtr<MTL::Function> _objectF;
        NS::SharedPtr<MTL::Function> _meshF;
        NS::SharedPtr<MTL::Function> _fragmentF;
    };
}

#endif //MESHPSO_HPP
