//
// Created by Giovanni Bollati on 18/06/25.
//

#ifndef IRENDERABLEFACTORY_HPP
#define IRENDERABLEFACTORY_HPP

#include <Rendering/IRenderable.hpp>
#include <Texture.hpp>

namespace Rendering
{
    class IRenderableFactory
    {
    public:
        virtual ~IRenderableFactory() = default;
        virtual std::shared_ptr<IRenderable> fromMesh(
            const Geometry::Mesh& mesh,
            const std::vector<std::shared_ptr<Texture>>& textures
            ) = 0;

        virtual std::shared_ptr<IRenderable> createProceduralMeshRenderable(
            const std::vector<std::shared_ptr<Texture>>& textures,
            const IRenderable::DispatchSize& gridSize,
            const IRenderable::DispatchSize& threadgroupSize
            ) = 0;
    };
}

#endif //IRENDERABLEFACTORY_HPP
