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

        // SET PIXEL FORMAT FOR MOTION VECTOR ATTACHMENT
        if (config.hasMotionVectors)
            meshPSODescriptor->colorAttachments()->object(1)->setPixelFormat(MTL::PixelFormat::PixelFormatRG16Float);


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

        // generate samplers
        for (int i = 0; i < config.samplers.size(); i++)
        {
            auto samplerD = config.samplers[i];
            auto samplerDescriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
            samplerDescriptor->setMinFilter(
                samplerD.descriptor.minFilter == Core::Linear ? MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilter::SamplerMinMagFilterNearest
            );
            samplerDescriptor->setMagFilter(
                samplerD.descriptor.magFilter == Core::Linear ? MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilter::SamplerMinMagFilterNearest
            );

            switch (samplerD.descriptor.addressModeU)
            {
            case Core::AddressMode::Repeat:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case Core::AddressMode::ClampToEdge:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case Core::AddressMode::ClampToZero:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case Core::AddressMode::MirrorRepeat:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeMirrorRepeat);
                break;
            default: break;
            }
            switch (samplerD.descriptor.addressModeV)
            {
            case Core::AddressMode::Repeat:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case Core::AddressMode::ClampToEdge:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case Core::AddressMode::ClampToZero:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case Core::AddressMode::MirrorRepeat:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeMirrorRepeat);
                break;
            default: break;
            }
            switch (samplerD.descriptor.addressModeW)
            {
            case Core::AddressMode::Repeat:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case Core::AddressMode::ClampToEdge:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case Core::AddressMode::ClampToZero:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case Core::AddressMode::MirrorRepeat:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeMirrorRepeat);
                break;
            default: break;
            }

            samplerDescriptor->setNormalizedCoordinates(samplerD.descriptor.normalizedCoordinates);
            auto sampler = NS::TransferPtr(device->newSamplerState(samplerDescriptor.get()));
            samplers[samplerD.descriptor] = sampler;
        }
    }
}
