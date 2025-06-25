//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef VERTEXDESCRIPTOR_HPP
#define VERTEXDESCRIPTOR_HPP

#include <unordered_map>
#include <vector>
#include <Core/VertexAttributeEnums.hpp>

namespace Rendering
{
    struct VertexAttribute
    {
        Core::VertexAttributeName name;
        Core::VertexAttributeType type;
        int attributeIndex; // index of the attribute in the buffer, used for Metal
    };

    /*
     * VertexDescriptor represents a collection of vertex attributes.
     * each element in the buffers vector represent a collection of attributes
     * those attributes should be set to the index of the buffers vector element
     * (for instance, the attribute at buffer[0][0] should be put in a metal buffer and set
     * to the buffer index 0 by the command encoder)
     * if the data must be interleaved, each element in the buffers vector
     * will contain more than one attribute.
     */
    struct VertexDescriptor
    {
        /*
         * Each buffer[i] is meant to be bound to a single buffer.
         * Each buffer[i] can contain more than one attribute;
         * if so, it means that the data of the attributes is
         * interleaved into a single buffer.
         * Eeach attribute has a name, a type and an index
        */
        std::vector<std::vector<VertexAttribute>> buffers; // each buffer contains a list of attributes

        bool isCompatibleWith(const VertexDescriptor& other) const;
        bool validateVertexDescriptor() const;
    };

    extern const std::unordered_map<std::string, const VertexDescriptor> G_getVertexDescriptors();
}

#endif //VERTEXDESCRIPTOR_HPP

