//
// Created by Giovanni Bollati on 21/06/25.
//

#ifndef RENDERERFACTORY_HPP
#define RENDERERFACTORY_HPP

#include <iostream>
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <Rendering/IRendererFactory.hpp>
#include <Rendering/Metal/Renderer.hpp>
#include <Rendering/Metal/PSOFactory.hpp>
#include <Rendering/Metal/RenderableFactory.hpp>

namespace Rendering::Metal
{
    class RendererFactory : public IRendererFactory
    {
    public:
        RendererFactory() = default;

        std::unique_ptr<IRenderer> createRenderer(
            SDL_Window* sdl_window
        ) override
        {
            auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
            if (!device) {
                throw std::runtime_error("Failed to create Metal device");
            }

            auto libraryPath = NS::Bundle::mainBundle()->resourcePath()->stringByAppendingString(
                NS::String::string("/Shaders.metallib", NS::ASCIIStringEncoding)
            );
            //std::cout << libraryPath->utf8String() << std::endl;
            NS::Error* error;
            auto library = NS::TransferPtr(device->newLibrary(libraryPath, &error));
            if (!library.get()) throw std::runtime_error(error->localizedDescription()->utf8String());

            return std::make_unique<Renderer>(
                sdl_window,
                device,
                library,
                std::make_unique<PSOFactory>(device.get(), library.get()),
                std::make_unique<RenderableFactory>()
            );
        }
    };
}

#endif //RENDERERFACTORY_HPP
