//
// Created by Giovanni Bollati on 20/06/25.
//

#ifndef IRENDERER_HPP
#define IRENDERER_HPP
#include <Rendering/PSOConfigs.hpp>
#include <Mesh.hpp>

#include <Rendering/IPSOFactory.hpp>
#include <Rendering/IRenderableFactory.hpp>

#include <glm/glm.hpp>

#include <Rendering/Lights.hpp>

namespace Rendering
{
    class IRenderer
    {
    public:
        IRenderer(
            std::unique_ptr<IPSOFactory> psoFactory,
            std::unique_ptr<IRenderableFactory> renderableFactory
        ) :
            _psoFactory(std::move(psoFactory)),
            _renderableFactory(std::move(renderableFactory)),
            _pipelineStateObjects(std::unordered_map<std::string, std::shared_ptr<IPSO>>()),
            _renderables(std::vector<std::shared_ptr<IRenderable>>()),
            _lights(Lights{})
        {}

        virtual ~IRenderer() = default;

        IRenderer(const IRenderer&) = delete;
        IRenderer& operator=(const IRenderer&) = delete;
        IRenderer(IRenderer&&) = delete;
        IRenderer& operator=(IRenderer&&) = delete;

        virtual void update(const glm::mat4x4& viewMatrix) = 0;

        void addDirectionalLight(const DirectionalLight& light);
        void addPointLight(const PointLight& light);
        void clearLights();
        void removeLastDirectionalLight();
        void removeLastPointLight();
        void setAmbientGlobalLight(const glm::vec4& color);
        void setLights(const Lights& lights);

        void addRenderable(const Mesh& mesh, const PSOConfig& psoConfig);
        void loadPSOs(const std::unordered_map<std::string, const PSOConfig>& psoConfigs);
        void loadPSO(const PSOConfig& config);


    protected:
        std::unique_ptr<IPSOFactory> _psoFactory;
        std::unique_ptr<IRenderableFactory> _renderableFactory;

        std::unordered_map<std::string, std::shared_ptr<IPSO>> _pipelineStateObjects;
        std::vector<std::shared_ptr<IRenderable>> _renderables;

        Lights _lights; // global lights structure, can be used by the renderables
    };
}

#endif //IRENDERER_HPP
