//
// Created by Giovanni Bollati on 10/06/25.
//

#ifndef VERTEXDESCRIPTORUTILS_HPP
#define VERTEXDESCRIPTORUTILS_HPP

#include "VertexDescriptor.hpp"
#include <Metal/MTLVertexDescriptor.hpp>

namespace Rendering
{
    NS::SharedPtr<MTL::VertexDescriptor> createVertexDescriptor(const VertexDescriptor& vertexDescriptor);
}

#endif //VERTEXDESCRIPTORUTILS_HPP
