//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef PSOCONFIG_HPP
#define PSOCONFIG_HPP

#include <string>
#include "VertexDescriptor.hpp"
#include "Material.hpp"

namespace Rendering
{
    enum PrimitiveType
    {
        Triangle,
        Line
    };
    enum FillMode
    {
        Solid,
        Wireframe
    };
    enum Culling
    {
        Front,
        Back,
        None
    };
    enum DepthTest
    {
        Enabled,
        Disabled
    };

    struct PSOConfig
    {
        std::string name;
        std::string vertexShader;
        std::string fragmentShader;
        PrimitiveType primitiveType;
        FillMode fillMode;
        Culling culling;
        DepthTest depthTest;
        VertexDescriptor vertexDescriptor;
        std::vector<MaterialInfo> materials;
    };

    extern const std::unordered_map<std::string, const PSOConfig> psoConfigs;
}

#endif //PSOCONFIG_HPP
