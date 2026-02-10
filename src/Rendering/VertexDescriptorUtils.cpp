//
// Created by Giovanni Bollati on 10/06/25.
//

#include "../../include/Rendering/VertexDescriptorUtils.hpp"
#include <Metal/MTLVertexDescriptor.hpp>
#include <Foundation/Foundation.hpp>
#include <simd/simd.h>

using namespace Core;
namespace Rendering
{
    // a metal vertex descriptor tells, for each attribute, from which metal buffer it should take it and set information
    // for interleaved data; with non interleaved data, each buffer starting from the first (0) takes one attribute, which
    // means that for n attibutes, the first n buffers are taken for vertex data
    NS::SharedPtr<MTL::VertexDescriptor> createVertexDescriptor(const VertexDescriptor& vertexDescriptor)
    {
        if (!vertexDescriptor.validateVertexDescriptor())
        {
            throw std::runtime_error("Invalid vertex descriptor");
        }
        // create metal vertex descriptor
        auto mtlVertexDescriptor = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());

        // loop through buffers
        for (size_t i = 0; i < vertexDescriptor.buffers.size(); ++i)
        {
            // i is the buffer index
            const auto& buffer = vertexDescriptor.buffers[i];
            int cumulativeStride = 0;

            // loop through attributes on a single buffer
            for (const auto& attribute : buffer)
            {
                // set buffer index for the attribute
                mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setBufferIndex(i);
                mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setOffset(cumulativeStride);

                // set the attribute format and increase cumulative stride (offset) if more than one attribute is going to be
                // set to the same buffer (interleaved data)
                switch (attribute.type)
                {
                case VertexAttributeType::Float3:
                    cumulativeStride += sizeof(simd::float3);
                    mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setFormat(MTL::VertexFormatFloat3);
                    break;
                case VertexAttributeType::Float4:
                    cumulativeStride += sizeof(simd::float4);
                    mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setFormat(MTL::VertexFormatFloat4);
                    break;
                case VertexAttributeType::Float2:
                    cumulativeStride += sizeof(simd::float2);
                    mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setFormat(MTL::VertexFormatFloat2);
                    break;
                default:
                    throw std::runtime_error("Unsupported vertex attribute type");
                }
            }
            // stride -> offsets sum
            mtlVertexDescriptor->layouts()->object(i)->setStride(cumulativeStride);
        }
        return mtlVertexDescriptor;
    }
}
