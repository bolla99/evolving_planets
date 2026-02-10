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
    /**
     * At its basic, a renderable holds a reference to a pipeline state object, a vertices count, faces count
     * and a collection of materials, which are simply vectors of bytes that should be bind according to material infos
     * Others members are optional and could be moved elsewhere; the derived class should holds the gpu buffers and should
     * be able to render itself by receiving the render command encoder and by having a reference to the right pso
     */
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
            // create materials
            for (const auto& materialInfo : pso->materials)
            {
                if (materialInfo.frequency == PerFrame) continue;
                _materialInfos.emplace_back(materialInfo);
                _materials.emplace_back(getDefaultBytes(materialInfo.type));
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

        void setMaterial(const std::vector<std::byte>& materialBytes, MaterialType type)
        {
            for (int i = 0; i < _materials.size(); i++)
            {
                if (_materialInfos[i].type == type)
                {
                    auto def = getDefaultBytes(type);
                    assert(def.size() == materialBytes.size() && "Material size mismatch");
                    _materials[i] = materialBytes;
                }
            }
        }

        // PUBLIC FIELDS
        bool visible = true;
        bool wireframe = false;

        std::shared_ptr<IPSO> _pso;

    protected:
        // PROTECTED FIELDS
        glm::mat4x4 _modelMatrix;
        int _verticesCount = 0;
        int _facesCount = 0;

        std::vector<std::vector<std::byte>> _materials;
        std::vector<MaterialInfo> _materialInfos;
    };
}


#endif //IRENDERABLE_HPP
