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

#include "RenderQueue.hpp"


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
            _nextRenderableID(1),
            _nextInstanceMaterialID(1),
            _freeIDs(std::vector<uint64_t>()),
            _renderables(std::unordered_map<uint64_t, std::shared_ptr<IRenderable>>()),
            _debugUICallback([]() {}),
            _drawableSize({800.0f, 600.0f})
        {}

        virtual ~IRenderer() = default;

        IRenderer(const IRenderer&) = delete;
        IRenderer& operator=(const IRenderer&) = delete;
        IRenderer(IRenderer&&) = delete;
        IRenderer& operator=(IRenderer&&) = delete;

        // UPDATE
        virtual void update(const RenderQueue& renderQueue, const glm::vec4& viewportNormalizedRect) = 0;

        // RENDERABLE API
        uint64_t addRenderable(
            const Geometry::Mesh& mesh,
            const std::vector<std::shared_ptr<Texture>>& textures,
            bool immediate = false
            );
        std::shared_ptr<IRenderable> removeRenderable(uint64_t index);

        // PSO API
        void loadPSOs(const std::unordered_map<std::string, const PSOConfig>& configs);
        void loadPSO(const PSOConfig& config);

        void setDebugUICallback(std::function<void()> callback);

        // get
        [[nodiscard]] glm::vec2 getDrawableSize() const { return _drawableSize; }

        // add a per object material instance creating default data for the given pso and return the id
        // the id will be used for changing data
        uint64_t addDefaultInstanceMaterial(const std::string& psoName, bool immediate = false);

        // if a material instance has that material type, then override with bytes
        void setInstanceMaterial(uint64_t id, const std::vector<std::byte>& bytes, MaterialType type);
        void removeInstanceMaterial(uint64_t id);

        // global material functions
        void setGlobalMaterial(const std::string& name, const std::vector<std::byte>& bytes, MaterialType type);
        void setGlobalMaterial(const std::vector<std::byte>& bytes, MaterialType type);

        template <typename T>
        std::optional<T> getGlobalMaterial(MaterialType type)
        {
            for (auto& [name, materials] : _globalMaterials)
            {
                for (auto& [materialType, materialBytes] : materials)
                {
                    if (materialType == type)
                    {
                        T t;
                        memcpy(&t, materialBytes.data(), sizeof(T));
                        return t;
                    }
                }
            }
            return std::nullopt;
        }

        std::pair<uint64_t, uint64_t> iDraw(
            const Geometry::Mesh& mesh,
            const std::vector<std::shared_ptr<Texture>>& textures,
            const std::string& psoName,
            const vector<pair<MaterialType, std::vector<std::byte>>>& materialOverrides,
            bool castShadows = false,
            bool wireframe = false,
            glm::ivec3 grid = {0, 0, 0},
            glm::ivec3 threadgroup = {0, 0, 0}
            );
        std::pair<uint64_t, uint64_t> iDrawRectWithColor(const glm::vec4& rect, float depth, const glm::vec4& color);
        std::pair<uint64_t, uint64_t> iDrawRectWithTex(const glm::vec4& rect, float depth, const std::vector<std::shared_ptr<Texture>>& textures, const std::string& pso);

        virtual void compute(
            const std::string& psoName,
            const std::vector<std::shared_ptr<Texture>>& inputTextures,
            const std::vector<std::shared_ptr<Texture>>& outputTextures,
            const std::vector<std::pair<MaterialType, std::vector<std::byte>>>& materials,
            glm::ivec3 grid, glm::ivec3 threadgroup
            ) = 0;

    protected:

        // FACTORIES
        std::unique_ptr<IPSOFactory> _psoFactory;
        std::unique_ptr<IRenderableFactory> _renderableFactory;

        // DATA
        std::unordered_map<uint64_t, std::shared_ptr<IRenderable>> _renderables;

        std::unordered_map<std::string, std::shared_ptr<IPSO>> _pipelineStateObjects;

        std::unordered_map<std::string, std::unordered_map<MaterialType, std::vector<std::byte>>> _globalMaterials;
        std::unordered_map<uint64_t, std::unordered_map<MaterialType, std::vector<std::byte>>> _instanceMaterials;

        std::unordered_map<std::string, std::unordered_map<TextureType, uint64_t>> _globalTextures;

        std::vector<uint64_t> _immediateRenderables;
        std::vector<uint64_t> _immediateInstanceMaterials;

        void setGlobalTexture(TextureType type, uint64_t);
        void setGlobalTexture(const std::string& name, TextureType type, uint64_t);
        virtual void destroyTexture(uint64_t) = 0;

        RenderQueue _immediateRenderQueue;
        // IDs
        uint64_t _nextRenderableID;
        uint64_t _nextInstanceMaterialID;
        std::vector<uint64_t> _freeIDs;
        std::vector<uint64_t> _freeInstanceMaterialIDs;

        std::function<void()> _debugUICallback;

        glm::vec2 _drawableSize;
    };
}

#endif //IRENDERER_HPP
