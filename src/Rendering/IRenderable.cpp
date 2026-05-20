//
// Created by Giovanni Bollati on 13/02/26.
//

#include <Rendering/IRenderable.hpp>
#include <Rendering/VertexDescriptor.hpp>

using namespace Rendering;

bool IRenderable::isCompatible(const VertexDescriptor& vd) const
{
    if (vd.isInterleaved()) throw std::runtime_error("Interleaved vertex descriptors not supported");
    // for each buffer in the vertex descriptor, check if there is an attribute in the renderable that matches
    for (auto& buffer : vd.buffers)
    {
        assert(buffer.size() == 1 && "vertex descriptor badly formatted");
        auto found = false;
        for (auto & vertexAttribute : _vertexAttributes)
        {
            assert(vertexAttribute.size() == 1 && "renderable attributes layout badly formatted");
            if (buffer[0].name == vertexAttribute[0].first and buffer[0].type == vertexAttribute[0].second)
            {
                // matching attribute found: exit inner loop and continue
                found = true; break;
            }
        }
        // no matching attribute found for this buffer form the vertex descriptor: return false
        if (not found) return false;
    }
    return true;
}