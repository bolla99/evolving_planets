//
// Created by Giovanni Bollati on 06/03/25.
//

#ifndef RENDERER_HPP
#define RENDERER_HPP

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <SDL2/SDL.h>
#include <SDL2/SDL_render.h>
#include <QuartzCore/QuartzCore.hpp>
#include <iostream>
#include <PipelineStateObject.hpp>
#include <GlobalEnums.hpp>
#include <PSOConfigs.hpp>
#include <Renderable.hpp>

#include "IRenderableLoader.hpp"

class Renderer
{
public:
    explicit Renderer(SDL_Window* sdl_window, std::unique_ptr<IRenderableLoader> renderableLoader);
    ~Renderer();

    void update() const;

    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer&) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&&) = delete;

    // map name to actual vertex descriptors
    std::unordered_map<VertexDescriptor, NS::SharedPtr<MTL::VertexDescriptor>> vertexDescriptors;
private:
    NS::SharedPtr<MTL::Device> _device;
    CA::MetalLayer* _layer;
    NS::SharedPtr<MTL::Library> _library;
    std::unique_ptr<IRenderableLoader> _renderableLoader;
    std::unordered_map<std::string, std::shared_ptr<PipelineStateObject>> _pipelineStateObjects;
    std::vector<std::shared_ptr<Renderable>> _renderables;

    SDL_Renderer* _sdl_renderer;

    void loadPSOs(const std::vector<PSOConfig>& psoConfigs);
    void createPSO(const PSOConfig& config);
    void setupVertexDescriptors();
};

#endif //RENDERER_HPP
