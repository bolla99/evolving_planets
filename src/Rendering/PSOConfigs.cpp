//
// Created by Giovanni Bollati on 09/06/25.
//

#include <Rendering/PSOConfigs.hpp>
#include <Rendering/VertexDescriptor.hpp>
#include <string>

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
        },
        std::pair<std::string, const PSOConfig>{
            std::string("VCPHONG_W"),
            PSOConfig
            {
                "VCPHONG_W",
                "vertexVCPHONG",
                "fragmentVCPHONG",
                Triangle,
                Wireframe,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PCN")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("UI"),
            PSOConfig
            {
                "UI",
                "vertexUI",
                "fragmentUI",
                Triangle,
                Solid,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PC")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("TexturePHONG"),
            PSOConfig
            {
                "TexturePHONG",
                "vertexTexturePHONG",
                "fragmentTexturePHONG",
                Triangle,
                Solid,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PCNUV")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("TextureUI"),
            PSOConfig
            {
                "TextureUI",
                "vertexTextureUI",
                "fragmentTextureUI",
                Triangle,
                Solid,
                Back,
                Disabled,
                G_getVertexDescriptors().at("PUV")
            }
        },
        std::pair<std::string, const PSOConfig>{
            std::string("curve"),
            PSOConfig
            {
                "curve",
                "vertexCurve",
                "fragmentCurve",
                Line,
                Solid,
                Back,
                Enabled,
                G_getVertexDescriptors().at("PC")
            }
        }
    };
}
