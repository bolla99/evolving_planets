//
// Created by Giovanni Bollati on 12/06/25.
//

#include "../../../include/Rendering/Metal/PSO.hpp"
#include <iostream>
#include <string>
#include "../../../include/Rendering/VertexDescriptorUtils.hpp"
#include <Texture.hpp>

using namespace Core;

namespace Rendering::Metal
{
    PSO::PSO(
        const PSOConfig& config,
        MTL::Device* device,
        MTL::Library* library
        ) : IPSO(config)
    {
        _vertexF = NS::TransferPtr(library->newFunction(
            NS::String::string(config.vertexShader.c_str(), NS::ASCIIStringEncoding)));
        if (!_vertexF)
        {
            throw std::runtime_error("Failed to create vertex function with name: " + config.vertexShader);
        }
        _fragmentF = NS::TransferPtr(library->newFunction(
            NS::String::string(config.fragmentShader.c_str(), NS::ASCIIStringEncoding)));
        if (!_fragmentF)
        {
            throw std::runtime_error("Failed to create fragment function with name: " + config.fragmentShader);
        }

        const auto psoDescriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
        if (!psoDescriptor)
        {
            throw std::runtime_error("Failed to create pipeline descriptor");
        }

        psoDescriptor->setVertexDescriptor(createVertexDescriptor(config.vertexDescriptor).get());

        psoDescriptor->setLabel(NS::String::string(config.name.c_str(), NS::ASCIIStringEncoding));
        psoDescriptor->setVertexFunction(_vertexF.get());
        psoDescriptor->setFragmentFunction(_fragmentF.get());

        // color attachment
        switch (config.colorPixelFormat)
        {
        case BGRAUnorm:
            psoDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);
            break;
        case ColorInvalid:
            psoDescriptor->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormat::PixelFormatInvalid);
            break;
        }

        // SET PIXEL FORMAT FOR MOTION VECTOR ATTACHMENT
        if (config.hasMotionVectors)
            psoDescriptor->colorAttachments()->object(1)->setPixelFormat(MTL::PixelFormat::PixelFormatRG16Float);


        if (config.depthTest == Enabled)
        {
            psoDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatDepth32Float);
        }
        else
        {
            psoDescriptor->setDepthAttachmentPixelFormat(MTL::PixelFormat::PixelFormatInvalid);
        }

        if (config.colorPixelFormat != ColorInvalid)
        {
            if (config.blendingMode == Additive)
            {
                psoDescriptor->colorAttachments()->object(0)->setBlendingEnabled(YES);
                psoDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
                psoDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
                psoDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
                psoDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOne);
                psoDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
                psoDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOne);
            }
             else if (config.blendingMode == Default)
             {
                 psoDescriptor->colorAttachments()->object(0)->setBlendingEnabled(YES);
                 psoDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
                 psoDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
                 psoDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
                 psoDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
                 psoDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
                 psoDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
             }
        }

        switch (config.primitiveType)
        {
            case Triangle:
                psoDescriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClass::PrimitiveTopologyClassTriangle);
                break;
            case Line:
                psoDescriptor->setInputPrimitiveTopology(MTL::PrimitiveTopologyClass::PrimitiveTopologyClassLine);
        }

        NS::Error* error;
        _metalPSO = NS::TransferPtr(device->newRenderPipelineState(psoDescriptor.get(), &error));

        if (!_metalPSO)
        {
            throw std::runtime_error(error->localizedDescription()->utf8String());
        }

         // generate samplers
        for (int i = 0; i < config.samplers.size(); i++)
        {
            auto samplerD = config.samplers[i];
            auto samplerDescriptor = NS::TransferPtr(MTL::SamplerDescriptor::alloc()->init());
            samplerDescriptor->setMinFilter(
                samplerD.descriptor.minFilter == Linear ? MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilter::SamplerMinMagFilterNearest
            );
            samplerDescriptor->setMagFilter(
                samplerD.descriptor.magFilter == Linear ? MTL::SamplerMinMagFilter::SamplerMinMagFilterLinear : MTL::SamplerMinMagFilter::SamplerMinMagFilterNearest
            );

            switch (samplerD.descriptor.addressModeU)
            {
            case AddressMode::Repeat:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case AddressMode::ClampToEdge:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case AddressMode::ClampToZero:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case AddressMode::MirrorRepeat:
                samplerDescriptor->setSAddressMode(MTL::SamplerAddressMode::SamplerAddressModeMirrorRepeat);
                break;
            default: break;
            }
            switch (samplerD.descriptor.addressModeV)
            {
            case AddressMode::Repeat:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case AddressMode::ClampToEdge:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case AddressMode::ClampToZero:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case AddressMode::MirrorRepeat:
                samplerDescriptor->setTAddressMode(MTL::SamplerAddressMode::SamplerAddressModeMirrorRepeat);
                break;
            default: break;
            }
            switch (samplerD.descriptor.addressModeW)
            {
            case AddressMode::Repeat:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeRepeat);
                break;
            case AddressMode::ClampToEdge:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToEdge);
                break;
            case AddressMode::ClampToZero:
                samplerDescriptor->setRAddressMode(MTL::SamplerAddressMode::SamplerAddressModeClampToZero);
                break;
            case AddressMode::MirrorRepeat:
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
