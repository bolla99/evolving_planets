//
// Created by Giovanni Bollati on 13/03/25.
//

#include "../include/PipelineStateObject.hpp"
#include <iostream>
#include <string>
#include "VertexDescriptorUtils.hpp"

PipelineStateObject::PipelineStateObject(
    const std::string& name,
    const std::string& vertexFunctionName,
    const std::string& fragmentFunctionName,
    MTL::Device* device,
    MTL::Library* library,
    MTL::PixelFormat pixelFormat,
    const VertexDescriptor& vertexDescriptor
    )
{
    _vertexF = NS::TransferPtr(library->newFunction(
        NS::String::string(vertexFunctionName.c_str(), NS::ASCIIStringEncoding)));
    if (!_vertexF)
    {
        throw std::runtime_error("Failed to create vertex function with name: " + vertexFunctionName);
    }
    _fragmentF = NS::TransferPtr(library->newFunction(
        NS::String::string(fragmentFunctionName.c_str(), NS::ASCIIStringEncoding)));
    if (!_fragmentF)
    {
        throw std::runtime_error("Failed to create fragment function with name: " + fragmentFunctionName);
    }

    auto psoDescriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    if (!psoDescriptor)
    {
        throw std::runtime_error("Failed to create pipeline descriptor");
    }


    psoDescriptor->setVertexDescriptor(createVertexDescriptor(vertexDescriptor).get());
    _vertexDescriptor = vertexDescriptor;

    psoDescriptor->setLabel(NS::String::string(name.c_str(), NS::ASCIIStringEncoding));
    psoDescriptor->setVertexFunction(_vertexF.get());
    psoDescriptor->setFragmentFunction(_fragmentF.get());
    psoDescriptor->colorAttachments()->object(0)->setPixelFormat(pixelFormat);

    NS::Error* error;
    _metalPSO = NS::TransferPtr(device->newRenderPipelineState(psoDescriptor.get(), &error));

    if (!_metalPSO)
    {
        throw std::runtime_error(error->localizedDescription()->utf8String());
    }
}
