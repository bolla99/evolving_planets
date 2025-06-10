//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef VERTEXDESCRIPTOR_HPP
#define VERTEXDESCRIPTOR_HPP

#include <vector>
#include <unordered_set>

enum VertexAttributeType
{
    Float3,
    Float4
};

enum VertexAttributeName
{
    Position,
    Color,
    Normal,
    Tangent,
    TexCoord
};

struct VertexAttribute
{
    VertexAttributeName name;
    VertexAttributeType type;
    int attributeIndex; // index of the attribute in the buffer, used for Metal
};

struct VertexDescriptor
{
    std::vector<std::vector<VertexAttribute>> buffers; // each buffer contains a list of attributes

    bool isCompatibleWith(const VertexDescriptor& other) const;
    bool validateVertexDescriptor() const;
};

#endif //VERTEXDESCRIPTOR_HPP

