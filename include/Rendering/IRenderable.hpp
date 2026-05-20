//
// Created by Giovanni Bollati on 12/06/25.
//

//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef IRENDERABLE_HPP
#define IRENDERABLE_HPP

#include <cstdint>
#include "ICommandEncoder.hpp"
#include "../Mesh.hpp"
#include "glm/mat4x4.hpp"

using namespace std;
using namespace Core;

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
        IRenderable() = delete;
        IRenderable(const IRenderable&) = delete;
        IRenderable& operator=(const IRenderable&) = delete;
        IRenderable(IRenderable&&) = delete;
        IRenderable& operator=(IRenderable&&) = delete;

        // CONSTRUCTOR
        IRenderable(
            int verticesCount,
            int facesCount,
            const vector<vector<pair<VertexAttributeName, VertexAttributeType>>>& vertexAttributes,
            const vector<TextureType>& texturesTypes
            ) :
            _verticesCount(verticesCount),
            _facesCount(facesCount),
            _vertexAttributes(vertexAttributes),
            _texturesTypes(texturesTypes),
            _gridSize({0, 0, 0}),
            _threadgroupSize({0, 0, 0})
        {}

        // DESTRUCTOR
        virtual ~IRenderable() = default;

        // Mesh shader dispatch dimensions
        struct DispatchSize { uint32_t x, y, z; };

        // RENDER FUNCTION
        virtual void render(
            ICommandEncoder* commandEncoder,
            IPSO* pso,
            uint instanceCount = 1,
            DispatchSize gridSize = {0, 0, 0},
            DispatchSize threadgroupSize = {0, 0, 0}
        ) const = 0;


        // verify compatibiliy between a vertex descriptor and the vertex layout
        // vertex descriptor is an object held by a pipeline state obejct
        [[nodiscard]] bool isCompatible(const VertexDescriptor& vd) const;

        void setDispatchDimensions(DispatchSize grid, DispatchSize threadgroup)
        {
            _gridSize = grid;
            _threadgroupSize = threadgroup;
        }

    protected:
        // PROTECTED FIELDS
        const int _verticesCount;
        const int _facesCount;

        // buffers metadata
        // {buffer 1: attribute 1, attribute 2, ...; buffer 2: attribute 1, attribute 2 ...}
        const vector<vector<pair<VertexAttributeName, VertexAttributeType>>> _vertexAttributes;
        std::vector<TextureType> _texturesTypes;

        // Mesh shader dispatch dimensions
        DispatchSize _gridSize;
        DispatchSize _threadgroupSize;
    };
}

#endif //IRENDERABLE_HPP