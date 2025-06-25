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
            ) :
        _vertexDescriptor(config.vertexDescriptor),
        primitiveType(config.primitiveType),
        fillMode(config.fillMode),
        culling(config.culling),
        depthTest(config.depthTest),
        name(config.name),
        vertexShader(config.vertexShader),
        fragmentShader(config.fragmentShader)
        {
            if (!_vertexDescriptor.validateVertexDescriptor())
            {
                throw std::runtime_error("Invalid vertex descriptor");
            }
        }

        // virtual destructor for polymorphism
        virtual ~IPSO() = default;
        // delete copy
        IPSO(const IPSO&) = delete;
        IPSO& operator=(const IPSO&) = delete;
        // delete move
        IPSO(IPSO&&) = delete;
        IPSO& operator=(IPSO&&) = delete;

        [[nodiscard]] virtual void* raw() const = 0;
        [[nodiscard]] const VertexDescriptor& getVertexDescriptor() const
        {
            return _vertexDescriptor;
        }

        const PrimitiveType primitiveType;
        const FillMode fillMode;
        const Culling culling;
        const DepthTest depthTest;
        const std::string name;
        const std::string vertexShader;
        const std::string fragmentShader;

    private:
        const VertexDescriptor& _vertexDescriptor;
    };
}

#endif //IPSO_HPP
