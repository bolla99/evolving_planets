//
// Created by Giovanni Bollati on 09/06/25.
//

#include "PSOConfigs.hpp"
#include "VertexDescriptor.hpp"

const std::vector<PSOConfig> psoConfigs = {
    {
        "triangle_pso",
        "vertexShader",
        "fragmentShader",
        {
                {
                    {
                        {Position, Float3, 0}
                    },
                    {
                        {Color, Float4, 1}
                    }
                }
        },
    },
    {
        "PC",
        "vertexColorShader",
        "fragmentColorShader",
        {
            {
                {
                    {Position, Float3, 0}
                }, {
                {Color, Float4, 1}
                    }
            }
        }
    }
};