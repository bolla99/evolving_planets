//
// Created by Giovanni Bollati on 10/06/25.
//


#include <Rendering/VertexDescriptor.hpp>

#include <string>
#include <Foundation/Foundation.hpp>
#include <unordered_set>

using namespace Core;
namespace Rendering
{
    const std::unordered_map<std::string, const VertexDescriptor> G_getVertexDescriptors() {
        static const  std::unordered_map<std::string, const VertexDescriptor> vertexDescriptors = {
            std::make_pair<std::string, const VertexDescriptor>(
                "triangle_pso",
                VertexDescriptor{
                    {
                        {
                            {Position, Float3, 0}
                        },
                        {
                            {Color, Float4, 1}
                        }
                    }
                }
            ),
            std::make_pair<std::string, const VertexDescriptor>(
                "PC",
                VertexDescriptor{
                {
                    {
                        {Position, Float3, 0}
                    },
                    {
                        {Color, Float4, 1}
                    }
                }
            }
        ),
            std::make_pair<std::string, const VertexDescriptor>(
                "PCN",
                VertexDescriptor{
                {
                    {
                        {Position, Float3, 0}
                    },
                    {
                        {Color, Float4, 1}
                    },
                    {
                        {Normal, Float3, 2}
                    }
                }
            }
        ),
            std::make_pair<std::string, const VertexDescriptor>(
                "PCNUV",
                VertexDescriptor{
                {
                    {
                        {Position, Float3, 0}
                    },
                    {
                        {Color, Float4, 1}
                    },
                    {
                        {Normal, Float3, 2}
                    },
                    {
                        {TexCoord, Float2, 3}
                    }
                }
            }
        ),
            std::make_pair<std::string, const VertexDescriptor>(
                "PUV",
                VertexDescriptor{
                {
                    {
                        {Position, Float3, 0}
                    },
                    {
                        {TexCoord, Float2, 1}
                    }
                }
            }
        )
    };
        return vertexDescriptors;
    }



    bool VertexDescriptor::isCompatibleWith(const VertexDescriptor& other) const
    {
        // check number of buffers
        if (buffers.size() != other.buffers.size())
        {
            return false;
        }
        for (size_t i = 0; i < buffers.size(); ++i)
        {
            // for each buffer, check the number of attributes
            if (buffers[i].size() != other.buffers[i].size())
            {
                return false;
            }
            // for each buffer, check that each attribute matches
            for (size_t j = 0; j < buffers[i].size(); ++j)
            {
                if (buffers[i][j].name != other.buffers[i][j].name ||
                    buffers[i][j].type != other.buffers[i][j].type ||
                    buffers[i][j].attributeIndex != other.buffers[i][j].attributeIndex
                    )
                {
                    return false;
                }
            }
        }
        return true;
    }

    bool VertexDescriptor::validateVertexDescriptor() const
    {
        auto indexes = std::unordered_set<int>();
        for (const auto& buffer : buffers)
        {
            for (const auto& attribute : buffer)
            {
                if (indexes.contains(attribute.attributeIndex))
                {
                    return false; // duplicate attribute index
                }
                indexes.insert(attribute.attributeIndex);
            }
        }
        for (int i = 0; i < indexes.size(); i++)
        {
            if (!indexes.contains(i))
            {
                return false; // missing attribute index
            }
        }
        return true;
    }
}