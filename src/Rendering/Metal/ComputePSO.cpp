//
// Created by Junie on 11/05/26.
//

#include "../../../include/Rendering/Metal/ComputePSO.hpp"
#include <iostream>
#include <string>

namespace Rendering::Metal
{
    ComputePSO::ComputePSO(
        const PSOConfig& config,
        MTL::Device* device,
        MTL::Library* library
        ) : IPSO(config)
    {
        _computeF = NS::TransferPtr(library->newFunction(
            NS::String::string(config.vertexShader.c_str(), NS::ASCIIStringEncoding)));
        if (!_computeF)
        {
            throw std::runtime_error("Failed to create compute function with name: " + config.vertexShader);
        }

        NS::Error* error;
        _metalPSO = NS::TransferPtr(device->newComputePipelineState(_computeF.get(), &error));

        if (!_metalPSO)
        {
            throw std::runtime_error(error->localizedDescription()->utf8String());
        }
    }
}
