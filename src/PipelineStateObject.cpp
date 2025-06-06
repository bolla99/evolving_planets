//
// Created by Giovanni Bollati on 13/03/25.
//

#include "../include/pso.hpp"
#include <iostream>
#include <string>

PipelineStateObject::PipelineStateObject(
    const std::string& name,
    const std::string& vertexName,
    const std::string& fragmentName,
    MTL::Device* device,
    MTL::Library* library,
    MTL::PixelFormat pixelFormat,
    MTL::VertexDescriptor* vertexDescriptor
    )
{
    _vertexF = NS::TransferPtr(library->newFunction(
        NS::String::string(vertexName.c_str(), NS::ASCIIStringEncoding)));
    if (!_vertexF)
    {
        throw std::runtime_error("Failed to create vertex function with name: " + vertex_name);
    }
    _fragmentF = NS::TransferPtr(library->newFunction(
        NS::String::string(fragmentName.c_str(), NS::ASCIIStringEncoding)));
    if (!_fragmentF)
    {
        throw std::runtime_error("Failed to create fragment function with name: " + fragment_name);
    }

    auto psoDescriptor = NS::TransferPtr(MTL::RenderPipelineDescriptor::alloc()->init());
    if (!psoDescriptor)
    {
        throw std::runtime_error("Failed to create pipeline descriptor");
    }

    if (vertexDescriptor)
    {
        psoDescriptor->setVertexDescriptor(vertexDescriptor);
        _vertexDescriptor = NS::TransferPtr(vertexDescriptor);
    }

    psoDescriptor->setLabel(NS::String::string(name.c_str(), NS::ASCIIStringEncoding));
    psoDescriptor->setVertexFunction(_vertexF.get());
    psoDescriptor->setFragmentFunction(_fragmentF.get());
    psoDescriptor->colorAttachments()->object(0)->setPixelFormat(pixel_format);

    NS::Error* error;
    _metalPSO = NS::TransferPtr(device->newRenderPipelineState(psoDescriptor.get(), &error));

    if (!_metalPSO)
    {
        throw std::runtime_error(error->localizedDescription()->utf8String());
    }
}
