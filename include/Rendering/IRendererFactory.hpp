//
// Created by Giovanni Bollati on 21/06/25.
//

#ifndef IRENDERERFACTORY_HPP
#define IRENDERERFACTORY_HPP
#include <SDL_video.h>

#include "IMeshLoader.hpp"
#include "IRenderer.hpp"

namespace Rendering
{
    class IRendererFactory
    {
    public:
        virtual ~IRendererFactory() = default;

        // Create a renderer with the given parameters
        virtual std::unique_ptr<IRenderer> createRenderer(
            SDL_Window* sdl_window
        ) = 0;
    };
}

#endif //IRENDERERFACTORY_HPP
