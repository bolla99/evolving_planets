//
// Created by Giovanni Bollati on 11/06/25.
//

#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>
#include <vector>
#include <Core/VertexAttributeEnums.hpp>

#include "glm/vec4.hpp"

class Mesh
{
public:
    Mesh(int numVertices, int numFaces,
         const std::vector<Core::VertexAttributeName>& vertexAttributeNames,
         const std::vector<Core::VertexAttributeType>& vertexAttributeTypes,
         const std::vector<std::vector<uint8_t>>& vertexData,
         const std::vector<uint32_t>& faces);

    Mesh(const Mesh& mesh) = delete;
    Mesh& operator=(const Mesh& mesh) = delete;
    Mesh(Mesh&& mesh) = delete;
    Mesh& operator=(Mesh&& mesh) = delete;

    [[nodiscard]] bool HasAttribute(Core::VertexAttributeName attributeName) const;
    [[nodiscard]] Core::VertexAttributeType GetAttributeType(Core::VertexAttributeName attributeName) const;

    // permit copy, not move
    [[nodiscard]] const std::vector<uint8_t>& getAttributeData(Core::VertexAttributeName attributeName) const;
    [[nodiscard]] const std::vector<uint32_t>& getFacesData() const;

    [[nodiscard]] int getNumVertices() const { return _numVertices; }
    [[nodiscard]] int getNumFaces() const { return _numFaces; }

    static std::shared_ptr<Mesh> quad(
        const float* pos, float w, float h, const float* color, float uvWidth, float uvHeight
        );
    static glm::vec4 noVertexColor()
    {
        return {1.0f, 0.0f, 1.0f, 1.0f};
    }


private:
    const int _numVertices;
    const int _numFaces;
    const std::vector<Core::VertexAttributeName> _vertexAttributeNames;
    const std::vector<Core::VertexAttributeType> _vertexAttributeTypes;
    const std::vector<std::vector<uint8_t>> _vertexData;
    const std::vector<uint32_t> _faces;
};

#endif //MESH_HPP
