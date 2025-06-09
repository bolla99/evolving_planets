//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef ASSIMPRENDERABLELOADER_HPP
#define ASSIMPRENDERABLELOADER_HPP
#include "IRenderableLoader.hpp"

class AssimpRenderableLoader : public IRenderableLoader
{
public:
    AssimpRenderableLoader() = default;
    std::shared_ptr<Renderable> loadRenderable(
        const std::string& path,
        std::shared_ptr<PipelineStateObject> pso,
        MTL::Device* device
        ) override;
};

#endif //ASSIMPRENDERABLELOADER_HPP
