//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef PSOCONFIG_HPP
#define PSOCONFIG_HPP

#include <string>
#include "VertexDescriptor.hpp"
#include "Material.hpp"
#include "Texture.hpp"

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

    enum ColorPixelFormat
    {
        BGRAUnorm,
        ColorInvalid
    };
    enum DepthPixelFormat
    {
        Depth32Float,
        DepthInvalid
    };

    enum class PipelineType
    {
        Vertex,
        Mesh,
        Compute
    };

    enum BlendingMode
    {
        Default,
        Additive
    };

    struct PSOConfig
    {
        std::string name;
        PipelineType pipelineType = PipelineType::Vertex;
        std::string vertexShader;      // For Vertex: vertex shader, For Mesh: object shader (optional)
        std::string fragmentShader;    // For Vertex: fragment shader, For Mesh: mesh shader
        std::string meshFragmentShader; // For Mesh: actual fragment shader
        PrimitiveType primitiveType;
        FillMode fillMode;
        Culling culling;
        DepthTest depthTest;
        BlendingMode blendingMode;
        ColorPixelFormat colorPixelFormat;
        DepthPixelFormat depthPixelFormat;
        VertexDescriptor vertexDescriptor;
        std::vector<MaterialInfo> instanceMaterials;
        std::vector<MaterialInfo> globalMaterials;
        std::vector<Core::TextureDescriptor> instanceTextures;
        std::vector<Core::TextureDescriptor> globalTextures;
        std::vector<Core::SamplerBinding> samplers;
        int indexBufferSlot = -1;
        bool hasMotionVectors = true;
    };

    extern const std::unordered_map<std::string, const PSOConfig> psoConfigs;
}

#endif //PSOCONFIG_HPP
