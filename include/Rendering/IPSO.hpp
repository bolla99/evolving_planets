//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef IPSO_HPP
#define IPSO_HPP
#include "VertexDescriptor.hpp"
#include "PSOConfigs.hpp"

namespace Rendering
{
    class IPSO
    {
    public:
        explicit IPSO(
            const PSOConfig& config
            ) : config(config)
        {
            if (!config.vertexDescriptor.validateVertexDescriptor())
            {
                throw std::runtime_error("Invalid vertex descriptor");
            }
        }

        // virtual destructor for polymorphism
        virtual ~IPSO() = default;

        IPSO(const IPSO&) = delete;
        IPSO& operator=(const IPSO&) = delete;
        IPSO(IPSO&&) = delete;
        IPSO& operator=(IPSO&&) = delete;

        [[nodiscard]] virtual void* raw() const = 0;

        const PSOConfig config;
    };
}

#endif //IPSO_HPP
