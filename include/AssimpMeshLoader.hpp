//
// Created by Giovanni Bollati on 11/06/25.
//

#ifndef ASSIMPMESHLOADER_HPP
#define ASSIMPMESHLOADER_HPP

#include <IMeshLoader.hpp>

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"
#include <Core/VertexAttributeEnums.hpp>

class AssimpMeshLoader : public IMeshLoader
{
public:
    std::shared_ptr<Mesh> loadMesh(const std::string& path) const override
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path,
            aiProcess_Triangulate | // Ensure all meshes are triangulated
            aiProcess_JoinIdenticalVertices // Join identical vertices
            );
        if (!scene)
        {
            throw std::runtime_error("aiScene is null: " + std::string(importer.GetErrorString()));
        }
        if (!scene->HasMeshes())
        {
            throw std::runtime_error("File read correctly, but no meshes found in the file: " + path);
        }
        auto mesh = scene->mMeshes[0];
        std::vector<Core::VertexAttributeName> vertexAttributeNames;
        std::vector<Core::VertexAttributeType> vertexAttributeTypes;
        std::vector<std::vector<uint8_t>> vertexData;
        std::vector<uint32_t> faces;
        int numVertices = mesh->mNumVertices;
        int numFaces = mesh->mNumFaces;
        int i = 0;
        if (mesh->HasPositions())
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Position);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(numVertices * sizeof(float) * 3));
            for (int j = 0; j < numVertices; ++j)
            {
                auto position = mesh->mVertices[j];
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 3, &position, sizeof(float) * 3);
            }
            ++i;
        }
        if (mesh->HasVertexColors(0))
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Color);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float4);
            vertexData.emplace_back(std::vector<uint8_t>(numVertices * sizeof(float) * 4));
            for (int j = 0; j < numVertices; ++j)
            {
                auto color = mesh->mColors[0][j];
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 4, &color, sizeof(float) * 4);
            }
            ++i;
        } else
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Color);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float4);
            vertexData.emplace_back(numVertices * sizeof(float) * 4);
            for (int j = 0; j < numVertices; ++j)
            {
                auto color = Mesh::noVertexColor();
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 4, &color, sizeof(float) * 4);
            }
            ++i;
        }
        if (mesh->HasNormals())
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Normal);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(numVertices * sizeof(float) * 3));
            for (int j = 0; j < numVertices; ++j)
            {
                auto normal = mesh->mNormals[j];
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 3, &normal, sizeof(float) * 3);
            }
            ++i;
        }
        if (mesh->HasTextureCoords(0))
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::TexCoord);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float2);
            vertexData.emplace_back(std::vector<uint8_t>(numVertices * sizeof(float) * 2));
            for (int j = 0; j < numVertices; ++j)
            {
                auto texCoord = mesh->mTextureCoords[0][j];
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 2, &texCoord, sizeof(float) * 2);
            }
            ++i;
        }
        if (mesh->HasTangentsAndBitangents())
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Tangent);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(numVertices * sizeof(float) * 3));
            for (int j = 0; j < numVertices; ++j)
            {
                auto tangent = mesh->mTangents[j];
                std::memcpy(vertexData[i].data() + j * sizeof(float) * 3, &tangent, sizeof(float) * 3);
            }
            ++i;
        }
        // Prepare index data
        faces.resize(numFaces * 3); // if the mesh is triangulated
        for (int j = 0; j < numFaces; ++j)
        {
            auto const face = mesh->mFaces[j];
            if (face.mNumIndices != 3)
            {
                throw std::runtime_error("Only triangle meshes are supported; triangulation went bad");
            }
            faces[j*3] = face.mIndices[0];
            faces[j*3 + 1] = face.mIndices[1];
            faces[j*3 + 2] = face.mIndices[2];
        }

        SDL_Log("Mesh loaded from path: %s\n"
                "Number of vertices: %d\nNumber of faces: %d\nVertex colors: %s\nUV: %s\nNormals: %s\nTangents: %s\n",
                path.c_str(), numVertices, mesh->mNumFaces,
                mesh->HasVertexColors(0) ? "yes" : "no",
                mesh->HasTextureCoords(0) ? "yes" : "no",
                mesh->HasNormals() ? "yes" : "no",
                mesh->HasTangentsAndBitangents() ? "yes" : "no"
                );

        // get textures
        if (scene->HasTextures())
        {
            SDL_Log("Mesh has textures, but this loader does not support them yet.");
        }
        else
        {
            SDL_Log("Mesh has no textures.");
        }

        return std::make_shared<Mesh>(
            numVertices,
            mesh->mNumFaces, // assuming triangles
            vertexAttributeNames,
            vertexAttributeTypes,
            vertexData,
            faces
        );
    }
};

#endif //ASSIMPMESHLOADER_HPP
