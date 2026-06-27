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

#include "assimp/Exporter.hpp"

class AssimpMeshLoader : public IMeshLoader
{
public:
    std::shared_ptr<Geometry::Mesh> loadMesh(const std::string& path) const override
    {
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
            path, 0
            );
        if (!scene)
        {
            throw std::runtime_error("aiScene is null: " + std::string(importer.GetErrorString()));
        }
        if (!scene->HasMeshes())
        {
            throw std::runtime_error("File read correctly, but no meshes found in the file: " + path);
        }

        unsigned int totalVertices = 0;
        unsigned int totalFaces = 0;
        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            totalVertices += scene->mMeshes[m]->mNumVertices;
            totalFaces += scene->mMeshes[m]->mNumFaces;
        }

        auto firstMesh = scene->mMeshes[0];
        bool hasPositions = firstMesh->HasPositions();
        bool hasColors = firstMesh->HasVertexColors(0);
        bool hasNormals = firstMesh->HasNormals();
        bool hasTexCoords = firstMesh->HasTextureCoords(0);
        bool hasTangents = firstMesh->HasTangentsAndBitangents();

        std::vector<Core::VertexAttributeName> vertexAttributeNames;
        std::vector<Core::VertexAttributeType> vertexAttributeTypes;
        std::vector<std::vector<uint8_t>> vertexData;
        std::vector<uint32_t> faces;

        int attrCount = 0;
        int posAttrIdx = -1;
        int colAttrIdx = -1;
        int normAttrIdx = -1;
        int texAttrIdx = -1;
        int tangAttrIdx = -1;

        if (hasPositions)
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Position);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(totalVertices * sizeof(float) * 3));
            posAttrIdx = attrCount++;
        }

        vertexAttributeNames.emplace_back(Core::VertexAttributeName::Color);
        vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float4);
        vertexData.emplace_back(std::vector<uint8_t>(totalVertices * sizeof(float) * 4));
        colAttrIdx = attrCount++;

        if (hasNormals)
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Normal);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(totalVertices * sizeof(float) * 3));
            normAttrIdx = attrCount++;
        }
        if (hasTexCoords)
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::TexCoord);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float2);
            vertexData.emplace_back(std::vector<uint8_t>(totalVertices * sizeof(float) * 2));
            texAttrIdx = attrCount++;
        }
        if (hasTangents)
        {
            vertexAttributeNames.emplace_back(Core::VertexAttributeName::Tangent);
            vertexAttributeTypes.emplace_back(Core::VertexAttributeType::Float3);
            vertexData.emplace_back(std::vector<uint8_t>(totalVertices * sizeof(float) * 3));
            tangAttrIdx = attrCount++;
        }

        faces.resize(totalFaces * 3);

        unsigned int vertexOffset = 0;
        unsigned int faceOffset = 0;

        for (unsigned int m = 0; m < scene->mNumMeshes; ++m)
        {
            auto mesh = scene->mMeshes[m];
            unsigned int numVertices = mesh->mNumVertices;
            unsigned int numFaces = mesh->mNumFaces;

            if (hasPositions && mesh->HasPositions())
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto position = mesh->mVertices[j];
                    std::memcpy(vertexData[posAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 3, &position, sizeof(float) * 3);
                }
            }
            if (mesh->HasVertexColors(0))
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto color = mesh->mColors[0][j];
                    std::memcpy(vertexData[colAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 4, &color, sizeof(float) * 4);
                }
            }
            else
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto color = Geometry::Mesh::noVertexColor();
                    std::memcpy(vertexData[colAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 4, &color, sizeof(float) * 4);
                }
            }
            if (hasNormals && mesh->HasNormals())
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto normal = mesh->mNormals[j];
                    std::memcpy(vertexData[normAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 3, &normal, sizeof(float) * 3);
                }
            }
            if (hasTexCoords && mesh->HasTextureCoords(0))
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto texCoord = mesh->mTextureCoords[0][j];
                    std::memcpy(vertexData[texAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 2, &texCoord, sizeof(float) * 2);
                }
            }
            if (hasTangents && mesh->HasTangentsAndBitangents())
            {
                for (unsigned int j = 0; j < numVertices; ++j)
                {
                    auto tangent = mesh->mTangents[j];
                    std::memcpy(vertexData[tangAttrIdx].data() + (vertexOffset + j) * sizeof(float) * 3, &tangent, sizeof(float) * 3);
                }
            }

            for (unsigned int j = 0; j < numFaces; ++j)
            {
                auto const face = mesh->mFaces[j];
                if (face.mNumIndices != 3)
                {
                    throw std::runtime_error("Only triangle meshes are supported; triangulation went bad");
                }
                faces[(faceOffset + j)*3] = face.mIndices[0] + vertexOffset;
                faces[(faceOffset + j)*3 + 1] = face.mIndices[1] + vertexOffset;
                faces[(faceOffset + j)*3 + 2] = face.mIndices[2] + vertexOffset;
            }

            vertexOffset += numVertices;
            faceOffset += numFaces;
        }

        SDL_Log("Mesh loaded from path: %s\n"
                "Number of submeshes combined: %d\n"
                "Number of vertices: %d\nNumber of faces: %d\nVertex colors: %s\nUV: %s\nNormals: %s\nTangents: %s\n",
                path.c_str(), scene->mNumMeshes, totalVertices, totalFaces,
                hasColors ? "yes" : "no",
                hasTexCoords ? "yes" : "no",
                hasNormals ? "yes" : "no",
                hasTangents ? "yes" : "no"
                );

        if (scene->HasTextures())
        {
            SDL_Log("Mesh has textures, but this loader does not support them yet.");
        }
        else
        {
            SDL_Log("Mesh has no textures.");
        }

        return std::make_shared<Geometry::Mesh>(
            totalVertices,
            totalFaces,
            vertexAttributeNames,
            vertexAttributeTypes,
            vertexData,
            faces
        );
    }

    void saveMesh(const std::string& path, std::shared_ptr<Geometry::Mesh> mesh) const override
    {
        Assimp::Exporter exporter;

        // build aiScene
        auto scene = new aiScene();
        scene->mNumMaterials = 1;
        scene->mMaterials = new aiMaterial*[1];
        scene->mMaterials[0] = new aiMaterial();
        scene->mRootNode = new aiNode();
        scene->mRootNode->mNumMeshes = 1;
        scene->mRootNode->mMeshes = new unsigned int[1];
        scene->mRootNode->mMeshes[0] = 0;
        // set n meshes = 1
        scene->mNumMeshes = 1;
        // create the meshes array
        scene->mMeshes = new aiMesh*[1];

        // create mesh and set to scene
        auto assimpMesh = new aiMesh();
        scene->mMeshes[0] = assimpMesh;

        // set primitive type to triangle
        assimpMesh->mPrimitiveTypes = aiPrimitiveType_TRIANGLE;

        assimpMesh->mNumVertices = static_cast<unsigned int>(mesh->getNumVertices());
        assimpMesh->mVertices = new aiVector3D[assimpMesh->mNumVertices];

        // fill the vertices
        for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
            assimpMesh->mVertices[i] = aiVector3D(mesh->getVertices()[i].x, mesh->getVertices()[i].y, mesh->getVertices()[i].z);

        // fill the normals
        if (mesh->HasAttribute(Core::Normal))
        {
            auto normals = mesh->getNormals();
            assimpMesh->mNormals = new aiVector3D[assimpMesh->mNumVertices];
            auto meshNormals = mesh->getNormals();
            for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
            {
                assimpMesh->mNormals[i] = aiVector3D(meshNormals[i].x, meshNormals[i].y, meshNormals[i].z);
            }
        }

        // fill the texture coordinates
        if (mesh->HasAttribute(Core::TexCoord))
        {
            auto meshTC = mesh->getTextureCoordinated();
            assimpMesh->mTextureCoords[0] = new aiVector3D[assimpMesh->mNumVertices];
            assimpMesh->mNumUVComponents[0] = 2;
            for (unsigned int i = 0; i < assimpMesh->mNumVertices; ++i)
                assimpMesh->mTextureCoords[0][i] = aiVector3D(meshTC[i].x, meshTC[i].y, 0.0f);
        }

        // add faces
        assimpMesh->mNumFaces = static_cast<unsigned int>(mesh->getNumFaces());
        assimpMesh->mFaces = new aiFace[assimpMesh->mNumFaces];
        for (unsigned int i = 0; i < assimpMesh->mNumFaces; ++i)
        {
            assimpMesh->mFaces[i].mNumIndices = 3;
            assimpMesh->mFaces[i].mIndices = new unsigned int[3];
            assimpMesh->mFaces[i].mIndices[0] = mesh->getFacesData()[i * 3];
            assimpMesh->mFaces[i].mIndices[1] = mesh->getFacesData()[i * 3 + 1];
            assimpMesh->mFaces[i].mIndices[2] = mesh->getFacesData()[i * 3 + 2];
        }
        auto ret = exporter.Export(scene, "obj", path);
        if (ret != aiReturn_SUCCESS) {
            std::cerr << "Export failed: " << exporter.GetErrorString() << std::endl;
        }
        delete scene;

    }
};

#endif //ASSIMPMESHLOADER_HPP
