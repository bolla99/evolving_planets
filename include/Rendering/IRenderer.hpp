//
// Created by Giovanni Bollati on 20/06/25.
//

#ifndef IRENDERER_HPP
#define IRENDERER_HPP

#include <string>
#include <Rendering/PSOConfigs.hpp>
#include <Mesh.hpp>

#include <Rendering/IPSOFactory.hpp>
#include <Rendering/IRenderableFactory.hpp>

#include <glm/glm.hpp>
#include <functional>
#include <Rendering/Lights.hpp>



namespace Rendering
{
    enum class RenderLayer
    {
        BACKGROUND,
        OPAQUE,
        TRANSPARENT,
        UI,
        TEXT
    };

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
            _nextRenderableID(0),
            _freeIDs(std::vector<uint64_t>()),
            _renderables(std::array<std::unordered_map<uint64_t, std::shared_ptr<IRenderable>>, 5>()),
            _lights(Lights{}),
            _debugUICallback([]() {}),
            _aspect({0.0f, 0.0f, 1.0f, 1.0f}),
            _drawableSize({100.0f, 100.0f})
        {}

        virtual ~IRenderer() = default;

        IRenderer(const IRenderer&) = delete;
        IRenderer& operator=(const IRenderer&) = delete;
        IRenderer(IRenderer&&) = delete;
        IRenderer& operator=(IRenderer&&) = delete;

        virtual void update(const glm::mat4x4& viewMatrix) = 0;


        void setDirectionalLight(const DirectionalLight& light, int index);
        void setPointLight(const PointLight& light, int index);
        void clearLights();
        void setAmbientGlobalLight(const glm::vec4& color);
        void setLights(const Lights& lights);

        uint64_t addRenderable(
            const Mesh& mesh,
            const PSOConfig& psoConfig,
            const std::vector<std::shared_ptr<Texture>>& textures,
            RenderLayer layer = RenderLayer::OPAQUE
            );

        std::shared_ptr<IRenderable> removeRenderable(uint64_t index);
        void setVisible(uint64_t index, bool visible);
        void setWireframe(uint64_t index, bool wireframe);

        template <typename T>
        std::shared_ptr<T> getMaterial(uint64_t id)
        {
            for (auto& layer : _renderables)
            {
                if (layer.contains(id))
                {
                    auto renderable = layer[id];
                    auto material = renderable->getMaterial<T>();
                    if (material)
                    {
                        return material;
                    }
                }
            }
            return nullptr;
        }

        const glm::mat4x4& modelMatrix(uint64_t index);
        void modelMatrix(uint64_t index, const glm::mat4x4& matrix);
        void loadPSOs(const std::unordered_map<std::string, const PSOConfig>& psoConfigs);
        void loadPSO(const PSOConfig& config);

        void setDebugUICallback(std::function<void()> callback);

        [[nodiscard]] virtual glm::mat4x4 getProjectionMatrix() const = 0;

        virtual void setAspect(const std::array<float, 4>& aspect) { _aspect = aspect; }


    protected:
        std::unique_ptr<IPSOFactory> _psoFactory;
        std::unique_ptr<IRenderableFactory> _renderableFactory;

        std::unordered_map<std::string, std::shared_ptr<IPSO>> _pipelineStateObjects;

        uint64_t _nextRenderableID;
        std::vector<uint64_t> _freeIDs;
        std::array<std::unordered_map<uint64_t, std::shared_ptr<IRenderable>>, 5> _renderables;

        Lights _lights; // global lights structure, can be used by the renderables

        std::function<void()> _debugUICallback;

        std::array<float, 4> _aspect;
        std::array<float, 2> _drawableSize;
    };
}

#endif //IRENDERER_HPP
