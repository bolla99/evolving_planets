//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef IRENDERABLE_HPP
#define IRENDERABLE_HPP

#include <cstdint>
#include "ICommandEncoder.hpp"
#include "../Mesh.hpp"
#include "glm/mat4x4.hpp"

namespace Rendering
{
    class IRenderable
    {
    public:
        IRenderable(
            std::shared_ptr<IPSO> pso,
            int verticesCount,
            int facesCount
            ) : _pso(std::move(pso)),
        _modelMatrix(glm::mat4x4(1.0f)), // identity matrix
        _verticesCount(verticesCount),
        _facesCount(facesCount) {}

        virtual ~IRenderable() = default;
        virtual void render(ICommandEncoder* commandEncoder, const glm::mat4x4& viewProjectionMatrix) const = 0;

    protected:
        std::shared_ptr<IPSO> _pso;
        glm::mat4x4 _modelMatrix;
        int _verticesCount = 0;
        int _facesCount = 0;
    };
}


#endif //IRENDERABLE_HPP
