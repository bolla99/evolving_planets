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
            const Mesh& mesh,
            std::shared_ptr<IPSO> pso,
            const std::vector<std::shared_ptr<Texture>>& textures
            ) = 0;
    };
}

#endif //IRENDERABLEFACTORY_HPP
