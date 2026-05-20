//
// Created by Giovanni Bollati on 21/06/25.
//

#include <iostream>
#include <Rendering/IRenderer.hpp>
#include <Mesh.hpp>
#include <ranges>
#include <string>

namespace Rendering
{
    uint64_t IRenderer::addRenderable(
        const Geometry::Mesh& mesh,
        const std::vector<std::shared_ptr<Texture>>& textures,
        bool immediate
    )
    {
        try
        {
            auto renderable = _renderableFactory->fromMesh(mesh, textures);
            if (!renderable)
            {
                std::cerr << "Failed to create renderable from mesh" << std::endl;
                throw std::runtime_error("Failed to create renderable from mesh");
            }
            // check if we have a free ID to recycle
            uint64_t id = 0;
            if (!_freeIDs.empty())
            {
                id = _freeIDs.back();
                _freeIDs.pop_back(); // recycle the ID
            } else {
                id = _nextRenderableID++;
            }
            _renderables.emplace(
                id,
                renderable);
            if (immediate)
            {
                _immediateRenderables.push_back(id);
            }
            return id; // return the index of the added renderable
        } catch (const std::exception& e)
        {
            std::cerr << "Error creating renderable from mesh: " << e.what() << std::endl;
            throw std::runtime_error("Failed to create renderable from mesh");
        }
    }

    std::shared_ptr<IRenderable> IRenderer::removeRenderable(uint64_t index)
    {
        if (_renderables.contains(index))
        {
            auto renderable = _renderables[index];
            _renderables.erase(index);
            // recycle the ID
            _freeIDs.push_back(index);
            return renderable;
        }
        return nullptr;
    }

    // load the pipeline state objects from the PSOConfigs using the pso factory
    void IRenderer::loadPSOs(const std::unordered_map<std::string, const PSOConfig>& configs)
    {
        for (const auto& val : configs | std::views::values)
        {
            loadPSO(val);
        }
    }

    void IRenderer::loadPSO(const PSOConfig& config)
    {
        if (_pipelineStateObjects.contains(config.name)) return;

        _pipelineStateObjects.emplace(
            config.name,
            _psoFactory->create(config)
        );
        if (_globalMaterials.contains(config.name)) return;

        auto material = std::unordered_map<MaterialType, std::vector<std::byte>>();

        // create pso default materials
        for (const auto& info : config.globalMaterials)
        {
            material.insert({info.type, getDefaultBytes(info.type)});
        }
        _globalMaterials.emplace(config.name, material);
    }

    void IRenderer::setDebugUICallback(std::function<void()> callback)
    {
        _debugUICallback = std::move(callback);
    }

    // add a per object material instance creating default data for the given pso and return the id
    // the id will be used for changing data
    uint64_t IRenderer::addDefaultInstanceMaterial(const std::string& psoName, bool immediate)
    {
        uint64_t materialInstanceID;
        if (!_freeInstanceMaterialIDs.empty())
        {
            materialInstanceID = _freeInstanceMaterialIDs.back();
            _freeInstanceMaterialIDs.pop_back();
        }
        else
        {
            materialInstanceID = _nextInstanceMaterialID++;
        }
        auto material = std::unordered_map<MaterialType, std::vector<std::byte>>();
        auto infos = _pipelineStateObjects.at(psoName)->config.instanceMaterials;
        for (auto& info : infos)
        {
            material.insert({info.type, getDefaultBytes(info.type)});
        }
        _instanceMaterials.insert({materialInstanceID, material});

        if (immediate)
        {
            _immediateInstanceMaterials.push_back(materialInstanceID);
        }
        return materialInstanceID;
    }

    // if material instance has that material type, then override with bytes
    void IRenderer::setInstanceMaterial(const uint64_t id, const std::vector<std::byte>& bytes, MaterialType type)
    {
        /*
        int index = getMaterialIndex(id, type);
        if (index == -1) return;
        setInstanceMaterial(id, bytes, index);
         */
        if (_instanceMaterials.contains(id)) {
            if (_instanceMaterials.at(id).contains(type)) {
                _instanceMaterials.at(id).at(type) = bytes;
            }
        }
    }

    void IRenderer::removeInstanceMaterial(uint64_t id)
    {
        if (_instanceMaterials.contains(id))
        {
            _freeInstanceMaterialIDs.push_back(id);
            _instanceMaterials.erase(id);
            //_instanceMaterialsPSONames.erase(id);
        }
    }

    void IRenderer::setGlobalMaterial(const std::string& name, const std::vector<std::byte>& bytes, MaterialType type)
    {
        if (_globalMaterials.contains(name))
        {
            if (_globalMaterials.at(name).contains(type))
            {
                _globalMaterials.at(name).at(type) = bytes;
            }
        }
    }

    void IRenderer::setGlobalMaterial(const std::vector<std::byte>& bytes, MaterialType type)
    {
        for (const auto& psoName : _globalMaterials | std::views::keys)
        {
            if (_globalMaterials.at(psoName).contains(type))
            {
                _globalMaterials.at(psoName).at(type) = bytes;
            }
        }
    }

    std::pair<uint64_t, uint64_t> IRenderer::iDraw(
        const Geometry::Mesh& mesh,
        const std::vector<std::shared_ptr<Texture>>& textures,
        const std::string& psoName, const vector<pair<MaterialType,
        std::vector<std::byte>>>& materialOverrides,
        bool castShadows,
        bool wireframe,
        glm::ivec3 grid,
        glm::ivec3 threadgroup
        )
    {
        auto renderable = addRenderable(mesh, textures, true);
        auto material = addDefaultInstanceMaterial(psoName, true);
        for (const auto& override : materialOverrides)
        {
            setInstanceMaterial(material, override.second, override.first);
        }
        const auto item = RenderItem(renderable, material, castShadows, wireframe, grid, threadgroup);
        _immediateRenderQueue.add(item, psoName, RenderLayer::OPAQUE);
        return {renderable, material};
    }

    std::pair<uint64_t, uint64_t> IRenderer::iDrawRectWithColor(const glm::vec4& rect, float depth, const glm::vec4& color)
    {
        auto mesh = Geometry::Mesh::quad({0.0f, 0.0f, 1.0f, 1.0f}, depth, color, 1.0, 1.0);
        return iDraw(*mesh, {}, "UI", {
            {MaterialType::RECT, getBytes(rect) }
        });
    }

    std::pair<uint64_t, uint64_t> IRenderer::iDrawRectWithTex(const glm::vec4& rect, float depth, const std::vector<std::shared_ptr<Texture>>& textures, const std::string& pso)
    {
        auto mesh = Geometry::Mesh::quad({0.0f, 0.0f, 1.0f, 1.0f}, depth, {1.0f, 0.0f, 1.0f, 1.0f}, 1.0f, 1.0f);
        return iDraw(*mesh, textures, pso, {
            {MaterialType::RECT, getBytes(rect) }
        });
    }

    // set this texture as global textre of type == type for every pso
    void IRenderer::setGlobalTexture(TextureType type, uint64_t id)
    {
        for (auto& pso : _pipelineStateObjects)
        {
            auto globalTexturesConfig = pso.second->config.globalTextures;
            for (auto & textureInfo : globalTexturesConfig)
            {
                if (textureInfo.type == type)
                {
                    setGlobalTexture(pso.first, type, id);
                }
            }
        }
    }

    // if there is already a texture of this type for this pso, then destroy last texture
    // and set the new one
    void IRenderer::setGlobalTexture(const std::string& name, TextureType type, uint64_t id)
    {
        if (_globalTextures.contains(name))
        {
            if (_globalTextures.at(name).contains(type))
            {
                auto oldID = _globalTextures.at(name).at(type);
                destroyTexture(oldID);
                _globalTextures.at(name).at(type) = id;
            }
            else
            {
                _globalTextures.at(name).insert({type, id});
            }
        }
        else
        {
            _globalTextures.insert({name, {{type, id}}});
        }
    }
}
