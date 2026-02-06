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
        // CONSTRUCTOR
        IRenderable(
            std::shared_ptr<IPSO> pso,
            int verticesCount,
            int facesCount
            ) :
        _modelMatrix(glm::mat4x4(1.0f)), // identity matrix
        _verticesCount(verticesCount),
        _facesCount(facesCount)
        {
            // create materials from infos
            for (const auto& materialInfo : pso->materials)
            {
                _materials.emplace_back(IMaterial::factory(materialInfo));
            }

            _pso = std::move(pso);
        }

        // DESTRUCTOR
        virtual ~IRenderable() = default;

        // RENDER FUNCTION
        virtual void render(ICommandEncoder* commandEncoder, const glm::mat4x4& viewProjectionMatrix) const = 0;

        // METHODS
        [[nodiscard]] glm::mat4x4 modelMatrix() const
        {
            return _modelMatrix;
        }
        void modelMatrix(const glm::mat4x4& matrix)
        {
            _modelMatrix = matrix;
        }

        template <typename T>
        std::shared_ptr<T> getMaterial()
        {
            for (const auto& mat : _materials)
            {
                auto material = IMaterial::get<T>(mat);
                if (material) return material;
            }
            return nullptr;
        }

        // PUBLIC FIELDS
        bool visible = true;
        bool wireframe = false;

    protected:
        // PROTECTED FIELDS
        std::shared_ptr<IPSO> _pso;
        glm::mat4x4 _modelMatrix;
        int _verticesCount = 0;
        int _facesCount = 0;

        std::vector<std::shared_ptr<IMaterial>> _materials;
    };
}


#endif //IRENDERABLE_HPP
