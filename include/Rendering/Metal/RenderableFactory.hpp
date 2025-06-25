//
// Created by Giovanni Bollati on 18/06/25.
//

#ifndef METALRENDERABLEFACTORY_HPP
#define METALRENDERABLEFACTORY_HPP
#include "../IRenderableFactory.hpp"
#include "PSO.hpp"
#include "Renderable.hpp"

namespace Rendering::Metal
{
    class RenderableFactory : public IRenderableFactory {
    public:
        RenderableFactory() = default;
        std::shared_ptr<IRenderable> fromMesh(const Mesh& mesh, std::shared_ptr<IPSO> pso) override
        {
            // cast IPSO to MetalPSO
            auto metalPSO = std::dynamic_pointer_cast<PSO>(pso);
            if (!metalPSO) {
                throw std::runtime_error("Invalid Pipeline State Object type: a MetalRenderableFactory expects a MetalPSO");
            }

            // get a vertex descriptor and check if it is valid
            auto vd = metalPSO->getVertexDescriptor();

            // prepare data
            auto data = std::vector<std::vector<float>>();
            auto bufferIndices = std::vector<int>();
            for(int i = 0; i < vd.buffers.size(); ++i) {
                if (vd.buffers[i].size() > 1) {
                    throw std::runtime_error("MetalRenderableFactory does not support interleaved vertex data at the moment.");
                }
                auto attribute = vd.buffers[i][0];
                if (mesh.HasAttribute(attribute.name))
                {
                    if (attribute.name == Core::VertexAttributeName::Normal)
                    {
                        SDL_Log("Normal attribute found.");
                    } else if (attribute.name == Core::VertexAttributeName::Position)
                    {
                        SDL_Log("Position attribute found.");
                    } else if (attribute.name == Core::VertexAttributeName::Color)
                    {
                        SDL_Log("Color attribute found.");
                    } else if (attribute.name == Core::VertexAttributeName::TexCoord)
                    {
                        SDL_Log("TexCoord attribute found.");
                    } else
                    {
                        SDL_Log("Unknown attribute found: %s", std::to_string(attribute.name).c_str());
                    }

                    std::vector<float> attributeDataFloat;
                    auto attributeData = mesh.getAttributeData(attribute.name);
                    attributeDataFloat.resize(attributeData.size());
                    memcpy(attributeDataFloat.data(), attributeData.data(), attributeData.size());

                    // here I should transform from a vector of uint8_t to a vector of simd::float2, 3 or 4 so that
                    // the padding is correct
                    if (attributeData.empty())
                    {
                        throw std::runtime_error("Mesh attribute data is empty for attribute: " + std::to_string(attribute.name));
                    }
                    if (attribute.type == Core::Float3)
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
                    else if (attribute.type == Core::Float4)
                    {
                        data.emplace_back(attributeDataFloat);
                    }
                    else
                    {
                        throw std::runtime_error("Unsupported vertex attribute type: " + std::to_string(attribute.type));
                    }
                    // the buffer indices is the index of the buffer this array will be binded to
                    // the indices are the positions in the buffers vector of the vertex descriptor
                    bufferIndices.emplace_back(i);
                }
                else
                {
                    throw std::runtime_error("Mesh does not have the required attribute: " + std::to_string(vd.buffers[i][0].name));
                }
            }
            SDL_Log("Renderable Factory: calling Renderable constructor.");
            return std::make_shared<Renderable>(
                data,
                mesh.getFacesData(),
                bufferIndices,
                metalPSO,
                mesh.getNumVertices(),
                mesh.getNumFaces()
                );
        }
    };
}



#endif //METALRENDERABLEFACTORY_HPP
