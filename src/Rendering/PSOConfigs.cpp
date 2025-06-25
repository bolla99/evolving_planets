//
// Created by Giovanni Bollati on 09/06/25.
//

#include <Rendering/PSOConfigs.hpp>
#include <Rendering/VertexDescriptor.hpp>

namespace Rendering
{
    const std::unordered_map<std::string, const PSOConfig> psoConfigs = {
        std::pair<std::string, const PSOConfig>{
            std::string("triangle_pso"),
            PSOConfig{
               "triangle_pso",
               "vertexShader",
               "fragmentShader",
               Triangle,
               Solid,
               Back,
               Enabled,
               G_getVertexDescriptors().at("triangle_pso")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("PC"),
            PSOConfig
            {
                "PC",
                "vertexColorShader",
                "fragmentColorShader",
                Triangle,
                Solid,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PC")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCPHONG"),
            PSOConfig
            {
                "VCPHONG",
                "vertexVCPHONG",
                "fragmentVCPHONG",
                Triangle,
                Solid,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PCN")
            }
        }
    };
}
