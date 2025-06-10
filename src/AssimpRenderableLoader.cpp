//
// Created by Giovanni Bollati on 09/06/25.
//

#include "AssimpRenderableLoader.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

std::shared_ptr<Renderable> AssimpRenderableLoader::loadRenderable(
        const std::string& path,
        std::shared_ptr<PipelineStateObject> pso,
        MTL::Device* device
        )
{
        auto vd = pso->_vertexDescriptor;

        // check if the vertex descriptor requires interleaved data
        for (const auto& buffer : vd.buffers)
        {
                if (buffer.size() > 1)
                {
                        throw std::runtime_error("AssimpRenderableLoader does not support interleaved vertex data.");
                }
        }

        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(
                path, aiProcess_Triangulate);
        if (!scene || !scene->HasMeshes())
        {
                throw std::runtime_error("Mesh at path: " + path + " could not be loaded.");
        }
        aiMesh* mesh = scene->mMeshes[0];

        auto data = std::vector<std::pair<int, void*>>();

        for (const auto& buffer : vd.buffers)
        {
                auto attributeData = buffer[0];
                if (attributeData.name != VertexAttributeName::Position &&
                    attributeData.name != VertexAttributeName::Color)
                {
                        throw std::runtime_error("AssimpRenderableLoader only supports Position and Color attributes.");
                }
                if (attributeData.name == Position)
                {
                        if (mesh->HasPositions())
                        {
                                auto vertices = std::vector<simd::float3>(mesh->mNumVertices);
                                for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
                                {
                                        vertices[i] = simd::make_float3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                                }
                                data.emplace_back(static_cast<int>(vertices.size() * sizeof(simd::float3)), vertices.data());
                        }
                        else
                        {
                                throw std::runtime_error("Mesh does not have position data.");
                        }
                }
                else if (attributeData.name == Color)
                {
                        if (mesh->HasVertexColors(0))
                        {
                                auto colors = std::vector<simd::float4>(mesh->mNumVertices);
                                for (unsigned int i = 0; i < mesh->mNumVertices; ++i)
                                {
                                        colors[i] = simd::make_float4(
                                                mesh->mColors[0][i].r,
                                                mesh->mColors[0][i].g,
                                                mesh->mColors[0][i].b,
                                                mesh->mColors[0][i].a
                                        );
                                }
                                data.emplace_back(static_cast<int>(colors.size() * sizeof(simd::float4)), colors.data());
                        }
                        else
                        {
                                throw std::runtime_error("Mesh does not have color data.");
                        }
                }

        }

        return std::make_shared<Renderable>(
                data,
                pso,
                static_cast<int>(mesh->mNumVertices),
                device
        );
}
