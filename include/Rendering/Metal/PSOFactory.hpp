//
// Created by Giovanni Bollati on 18/06/25.
//

#ifndef METALPSOFACTORY_HPP
#define METALPSOFACTORY_HPP
#include "../IPSO.hpp"
#include "../IPSOFactory.hpp"
#include "PSO.hpp"
#include "../PSOConfigs.hpp"
#include "../../Metal/MTLDevice.hpp"

namespace Rendering::Metal
{
    class PSOFactory : public IPSOFactory
    {
    public:
        PSOFactory(MTL::Device* device, MTL::Library* library)
            : _device(device), _library(library)
        {}
        std::shared_ptr<IPSO> create(const PSOConfig& config) override
        {
            return std::make_shared<PSO>
            (
                config,
                _device,
                _library,
                MTL::PixelFormatBGRA8Unorm // Example pixel format
            );
        }

    private:
        MTL::Device* _device; // unmanaged
        MTL::Library* _library; // unmanaged
    };
}

#endif //METALPSOFACTORY_HPP
