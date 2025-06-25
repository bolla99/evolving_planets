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
    NS::SharedPtr<MTL::VertexDescriptor> createVertexDescriptor(const VertexDescriptor& vertexDescriptor)
    {
        if (!vertexDescriptor.validateVertexDescriptor())
        {
            throw std::runtime_error("Invalid vertex descriptor");
        }
        // create metal vertex descriptor
        auto mtlVertexDescriptor = NS::TransferPtr(MTL::VertexDescriptor::alloc()->init());

        for (size_t i = 0; i < vertexDescriptor.buffers.size(); ++i)
        {
            const auto& buffer = vertexDescriptor.buffers[i];
            int cumulativeStride = 0;

            for (const auto& attribute : buffer)
            {
                // set buffer index for the attribute
                mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setBufferIndex(i);
                mtlVertexDescriptor->attributes()->object(attribute.attributeIndex)->setOffset(cumulativeStride);

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
                default:
                    throw std::runtime_error("Unsupported vertex attribute type");
                }}
            mtlVertexDescriptor->layouts()->object(i)->setStride(cumulativeStride);
        }
        return mtlVertexDescriptor;
    }
}
