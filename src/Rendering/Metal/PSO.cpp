//
// Created by Giovanni Bollati on 12/06/25.
//

#include "../../../include/Rendering/Metal/PSO.hpp"
#include <iostream>
#include <string>
#include "../../../include/Rendering/VertexDescriptorUtils.hpp"

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
            psoDescriptor->colorAttachments()->object(0)->setBlendingEnabled(YES);
            psoDescriptor->colorAttachments()->object(0)->setRgbBlendOperation(MTL::BlendOperationAdd);
            psoDescriptor->colorAttachments()->object(0)->setAlphaBlendOperation(MTL::BlendOperationAdd);
            psoDescriptor->colorAttachments()->object(0)->setSourceRGBBlendFactor(MTL::BlendFactorSourceAlpha);
            psoDescriptor->colorAttachments()->object(0)->setDestinationRGBBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
            psoDescriptor->colorAttachments()->object(0)->setSourceAlphaBlendFactor(MTL::BlendFactorSourceAlpha);
            psoDescriptor->colorAttachments()->object(0)->setDestinationAlphaBlendFactor(MTL::BlendFactorOneMinusSourceAlpha);
        }
        psoDescriptor->setSampleCount(config.sampleCount);

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
    }
}
