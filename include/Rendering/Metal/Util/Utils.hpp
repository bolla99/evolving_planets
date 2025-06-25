//
// Created by Giovanni Bollati on 22/06/25.
//

#ifndef UTILS_HPP
#define UTILS_HPP

#include <simd/simd.h>
#include <glm/mat4x4.hpp>

#include "glm/gtc/type_ptr.hpp"

namespace Rendering::Metal::Util
{
    inline simd::float4x4 fromGLMtoSIMD(const glm::mat4x4& glmMatrix)
    {
        /*
        return simd::float4x4(
            simd::make_float4(glmMatrix[0][0], glmMatrix[1][0], glmMatrix[2][0], glmMatrix[3][0]),
            simd::make_float4(glmMatrix[0][1], glmMatrix[1][1], glmMatrix[2][1], glmMatrix[3][1]),
            simd::make_float4(glmMatrix[0][2], glmMatrix[1][2], glmMatrix[2][2], glmMatrix[3][2]),
            simd::make_float4(glmMatrix[0][3], glmMatrix[1][3], glmMatrix[2][3], glmMatrix[3][3])
        );
        */
        simd::float4x4 result;
        glm::mat4 t = glm::transpose(glmMatrix);
        std::memcpy(&result, glm::value_ptr(t), sizeof(float) * 16);
        return result;
    }
}

#endif //UTILS_HPP
