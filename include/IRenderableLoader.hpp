//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef RENDERABLELOADER_HPP
#define RENDERABLELOADER_HPP

#include <Renderable.hpp>

class IRenderableLoader
{
public:
    virtual ~IRenderableLoader() = default;
    virtual std::shared_ptr<Renderable> loadRenderable(
        const std::string& path,
        std::shared_ptr<PipelineStateObject> pso,
        MTL::Device* device
        ) = 0;
};

#endif //RENDERABLELOADER_HPP
