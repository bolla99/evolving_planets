//
// Created by Giovanni Bollati on 11/06/25.
//
#include "Mesh.hpp"



Mesh::Mesh(int numVertices, int numFaces,
     const std::vector<Core::VertexAttributeName>& vertexAttributeNames,
     const std::vector<Core::VertexAttributeType>& vertexAttributeTypes,
     const std::vector<std::vector<uint8_t>>& vertexData,
     const std::vector<uint32_t>& faces) : _numVertices(numVertices),
                                                            _numFaces(numFaces),
                                                            _vertexAttributeNames(vertexAttributeNames),
                                                            _vertexAttributeTypes(vertexAttributeTypes),
                                                            _vertexData(vertexData),
                                                            _faces(faces) {}

bool Mesh::HasAttribute(Core::VertexAttributeName attributeName) const
{
    for (auto& name : _vertexAttributeNames)
    {
        if (name == attributeName)
        {
            return true;
        }
    }
    return false;
}

Core::VertexAttributeType Mesh::GetAttributeType(Core::VertexAttributeName attributeName) const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == attributeName)
        {
            return _vertexAttributeTypes[i];
        }
    }
    throw std::runtime_error("Attribute not found");
}

const std::vector<uint8_t>& Mesh::getAttributeData(Core::VertexAttributeName attributeName) const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == attributeName)
        {
            return _vertexData[i];
        }
    }
    throw std::runtime_error("Attribute data not found");
}

const std::vector<uint32_t>& Mesh::getFacesData() const
{
    return _faces;
}
