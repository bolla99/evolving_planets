//
// Created by Junie on 11/05/26.
//

#ifndef METALCOMPUTEPSO_HPP
#define METALCOMPUTEPSO_HPP

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <Rendering/IPSO.hpp>

namespace Rendering::Metal
{
    class ComputePSO : public IPSO
    {
    public:
        ComputePSO(
            const PSOConfig& config,
            MTL::Device* device,
            MTL::Library* library
            );

        [[nodiscard]] void* raw() const override { return _metalPSO.get(); }

        [[nodiscard]] MTL::Function* getComputeFunction() const { return _computeF.get(); }

    private:
        NS::SharedPtr<MTL::ComputePipelineState> _metalPSO;
        NS::SharedPtr<MTL::Function> _computeF;
    };
}

#endif //METALCOMPUTEPSO_HPP
