//
// Created by Giovanni Bollati on 18/06/25.
//

#ifndef IPSOFACTORY_HPP
#define IPSOFACTORY_HPP

#include "IPSO.hpp"
#include "PSOConfigs.hpp"

namespace Rendering
{
    class IPSOFactory
    {
    public:
        virtual ~IPSOFactory() = default;
        virtual std::shared_ptr<IPSO> create(const PSOConfig& config) = 0;
    };
}

#endif //IPSOFACTORY_HPP
