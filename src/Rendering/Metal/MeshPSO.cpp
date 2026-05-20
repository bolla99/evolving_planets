//
// Created by Giovanni Bollati on 21/04/26.
//

#include "../../../include/Rendering/Metal/MeshPSO.hpp"
#include <iostream>
#include <string>

namespace Rendering::Metal
{
    MeshPSO::MeshPSO(
        const PSOConfig& config,
        MTL::Device* device,
        MTL::Library* library
    ) : IPSO(config)
    {
        // Load object function (optional - for culling)
        if (!config.vertexShader.empty())
        {
            _objectF = NS::TransferPtr(library->newFunction(
                NS::String::string(config.vertexShader.c_str(), NS::ASCIIStringEncoding)));
            if (!_objectF)
            {
                throw std::runtime_error("Failed to create object function with name: " + config.vertexShader);
            }
        }

        // Load mesh function (required)
        if (config.fragmentShader.empty())
        {
            throw std::runtime_error("Mesh shader name is required for MeshPSO (should be in fragmentShader slot)");
        }
        _meshF = NS::TransferPtr(library->newFunction(
            NS::String::string(config.fragmentShader.c_str(), NS::ASCIIStringEncoding)));
        if (!_meshF)
        {
            std::cerr << "MeshPSO: Failed to find mesh function: " << config.fragmentShader << std::endl;
            throw std::runtime_error("Failed to create mesh function with name: " + config.fragmentShader);
        }

        // Load fragment function (required)
        if (config.meshFragmentShader.empty())
        {
            throw std::runtime_error("Fragment shader name is required for MeshPSO (should be in meshFragmentShader slot)");
        }
        _fragmentF = NS::TransferPtr(library->newFunction(
            NS::String::string(config.meshFragmentShader.c_str(), NS::ASCIIStringEncoding)));
        if (!_fragmentF)
        {
            std::cerr << "MeshPSO: Failed to find fragment function: " << config.meshFragmentShader << std::endl;
            throw std::runtime_error("Failed to create fragment function with name: " + config.meshFragmentShader);
        }

        // Create mesh render pipeline descriptor
        const auto meshPSODescriptor = NS::TransferPtr(MTL::MeshRenderPipelineDescriptor::alloc()->init());
        if (!meshPSODescriptor)
        {
            throw std::runtime_error("Failed to create mesh pipeline descriptor");
        }

        meshPSODescriptor->setLabel(NS::String::string(config.name.c_str(), NS::ASCIIStringEncoding));

        // Set functions
        if (_objectF)
        {
            meshPSODescriptor->setObjectFunction(_objectF.get());
        }
        meshPSODescriptor->setMeshFunction(_meshF.get());
        meshPSODescriptor->setFragmentFunction(_fragmentF.get());

        // Color attachment
        switch (config.colorPixelFormat)
        {
        case BGRAUnorm:
            meshPSODescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            break;
        case ColorInvalid:
            meshPSODescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatInvalid);
            break;
        }

        // Depth attachment
        if (config.depthTest == Enabled)
        {
            meshPSODescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth32Float);
        }
        else
        {
            meshPSODescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatInvalid);
        }

        // Blending
        if (config.colorPixelFormat != ColorInvalid)
        {
            meshPSODescriptor->colorAttachments()->object(0)->setBlendingEnabled(YES);
            meshPSODescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
            meshPSODescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
            meshPSODescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
            meshPSODescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            meshPSODescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
            meshPSODescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        }

        // Set max threadgroup sizes (Metal requirement for mesh shaders)
        meshPSODescriptor->setMaxTotalThreadsPerObjectThreadgroup(256);
        meshPSODescriptor->setMaxTotalThreadsPerMeshThreadgroup(64);

        meshPSODescriptor->setRasterSampleCount(config.sampleCount);

        // Create pipeline state
        NS::Error* error = nullptr;
        _metalPSO = NS::TransferPtr(device->newRenderPipelineState(meshPSODescriptor.get(), MTL::PipelineOptionNone, nullptr, &error));

        if (!_metalPSO)
        {
            std::string errorMsg = "Failed to create mesh pipeline state";
            if (error)
            {
                errorMsg += ": " + std::string(error->localizedDescription()->utf8String());
            }
            throw std::runtime_error(errorMsg);
        }
    }
}
