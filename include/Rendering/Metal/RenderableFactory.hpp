//
// Created by Giovanni Bollati on 18/06/25.
//

#ifndef METALRENDERABLEFACTORY_HPP
#define METALRENDERABLEFACTORY_HPP
#include "../IRenderableFactory.hpp"
#include "PSO.hpp"
#include "Renderable.hpp"
#include "../../../assimp/code/AssetLib/3MF/3MFXmlTags.h"

namespace Rendering::Metal
{
    class RenderableFactory : public IRenderableFactory {
    public:
        MTL::Device* device;
        RenderableFactory(MTL::Device* device) : device(device)
        {
            assert(device && "Rendering::Metal::RenderableFactory -> device is null during renderable factory construction");
        }

        std::shared_ptr<IRenderable> fromMesh(
            const Geometry::Mesh& mesh,
            const std::vector<std::shared_ptr<Texture>>& textures
            ) override
        {
            assert(device && "Rendering::Metal::RenderableFactory -> device is null during renderable construction");
            // prepare data
            auto data = std::vector<std::vector<float>>();
            auto vertexAttributes = std::vector<std::vector<std::pair<Core::VertexAttributeName, Core::VertexAttributeType>>>();

            for (auto attribute : mesh.getAttributes()) {
                std::vector<float> attributeDataFloat;
                auto attributeData = mesh.getAttributeData(attribute);
                attributeDataFloat.resize(attributeData.size());
                memcpy(attributeDataFloat.data(), attributeData.data(), attributeData.size());

                // here I should transform from a vector of uint8_t to a vector of simd::float2, 3 or 4 so that
                // the padding is correct
                if (attributeData.empty())
                {
                    std::cerr << "Rendering::Metal::RenderableFactory -> Mesh attribute data is empty for attribute: " + std::to_string(attribute) << std::endl;
                    throw std::runtime_error("Rendering::Metal::RenderableFactory -> Mesh attribute data is empty for attribute: " + std::to_string(attribute));
                }
                if (mesh.GetAttributeType(attribute) == Core::Float3)
                {
                    std::vector<float> padded;
                    padded.reserve(attributeDataFloat.size() / 3 * 4);
                    for (size_t j = 0; j < attributeDataFloat.size(); j += 3) {
                        padded.push_back(attributeDataFloat[j]);
                        padded.push_back(attributeDataFloat[j + 1]);
                        padded.push_back(attributeDataFloat[j + 2]);
                        padded.push_back(0.0f); // padding
                    }
                    data.emplace_back(padded);
                }
                else if (mesh.GetAttributeType(attribute) == Core::Float4)
                {
                    data.emplace_back(attributeDataFloat);
                }
                else if (mesh.GetAttributeType(attribute) == Core::Float2)
                {
                    data.emplace_back(attributeDataFloat);
                }
                else
                {
                    std::cerr << "Rendering::Metal::RenderableFactory -> Unsupported vertex attribute type: " + std::to_string(mesh.GetAttributeType(attribute)) << std::endl;
                    throw std::runtime_error("Rendering::Metal::RenderableFactory -> Unsupported vertex attribute type: " + std::to_string(mesh.GetAttributeType(attribute)));
                }
                // the buffer indices is the index of the buffer this array will be binded to
                // the indices are the positions in the buffers vector of the vertex descriptor
                vertexAttributes.push_back({{attribute, mesh.GetAttributeType(attribute)}});
            }

            // set textures type
            auto texturesTypes = std::vector<Core::TextureType>();
            for (const auto& texture : textures)
            {
                if (texture)
                    texturesTypes.emplace_back(texture->type());
            }

            //SDL_Log("Renderable Factory: calling Renderable constructor.");
            return std::make_shared<Renderable>(
            data,
            mesh.getFacesData(),
            mesh.getNumVertices(),
            mesh.getNumFaces(),
            textures,
            vertexAttributes,
            texturesTypes,
            device
            );
        }

        std::shared_ptr<IRenderable> createProceduralMeshRenderable(
            const std::vector<std::shared_ptr<Texture>>& textures,
            const IRenderable::DispatchSize& gridSize,
            const IRenderable::DispatchSize& threadgroupSize
        ) override
        {
            auto texturesTypes = std::vector<Core::TextureType>();
            for (const auto& texture : textures)
            {
                texturesTypes.emplace_back(texture->type());
            }

            auto renderable = std::make_shared<Renderable>(
                std::vector<std::vector<float>>{},
                std::vector<uint32_t>{},
                0, 0,
                textures,
                std::vector<std::vector<std::pair<Core::VertexAttributeName, Core::VertexAttributeType>>>{},
                texturesTypes,
                device
            );
            renderable->setDispatchDimensions(gridSize, threadgroupSize);
            return renderable;
        }
    };
}



#endif //METALRENDERABLEFACTORY_HPP
