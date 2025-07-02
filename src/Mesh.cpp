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

std::shared_ptr<Mesh> Mesh::quad(
        const float* pos, const float w, const float h, const float* color, float uvWidth, float uvHeight
        )
{
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::TexCoord
    };

    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float2
    };

    std::vector<std::vector<uint8_t>> vertexData(3);

    // Position data for 4 vertices
    std::vector<float> positions = {
        pos[0], pos[1], pos[2], // v0
        pos[0] + w, pos[1], pos[2], // v1
        pos[0] + w, pos[1] + h, pos[2], // v2
        pos[0], pos[1] + h, pos[2] // v3
    };

    vertexData[0].resize(positions.size() * sizeof(float));
    std::memcpy(vertexData[0].data(), positions.data(), vertexData[0].size());

    // Color data for 4 vertices
    std::vector<float> colors = {
        color[0], color[1], color[2], color[3], // v0
        color[0], color[1], color[2], color[3], // v1
        color[0], color[1], color[2], color[3], // v2
        color[0], color[1], color[2], color[3] // v3
    };
    vertexData[1].resize(colors.size() * sizeof(float));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());

    if (uvWidth < 0.0f || uvWidth > 1.0f)
    {
        throw std::invalid_argument("uvWidth must be in the range [0.0, 1.0]");
    }

    std::vector<float> uvs = {
        0.0f, 0.0f, // v0
        uvWidth, 0.0f, // v1
        uvWidth, uvHeight, // v2
        0.0f, uvHeight // v3
    };

    vertexData[2].resize(uvs.size() * sizeof(float));
    std::memcpy(vertexData[2].data(), uvs.data(), vertexData[2].size());


    // Face indices for 2 triangles
    std::vector<uint32_t> faces = {
        0, 1, 2, // First triangle
        0, 2, 3 // Second triangle
    };

    return std::make_shared<Mesh>(4, 2, attributeNames, attributeTypes, vertexData, faces);
}
