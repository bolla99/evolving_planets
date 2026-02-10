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
        const Mesh& mesh,
        const PSOConfig& psoConfig,
        const std::vector<std::shared_ptr<Texture>>& textures,
        RenderLayer layer
    )
    {
        if (!_pipelineStateObjects.contains(psoConfig.name))
        {
            try
            {
                loadPSO(psoConfig);
            } catch (const std::exception& e)
            {
                std::cerr << "Error creating PSO with name " << psoConfig.name << ": " << e.what() << std::endl;
                throw std::runtime_error("Failed to create PSO for renderable");
            }
        }

        // create the renderable from the mesh and the pso
        try
        {
            auto renderable = _renderableFactory->fromMesh(mesh, _pipelineStateObjects.at(psoConfig.name), textures);
            if (!renderable)
            {
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
            _renderables[static_cast<int>(layer)].emplace(
                id,
                renderable);
            return id; // return the index of the added renderable
        } catch (const std::exception& e)
        {
            std::cerr << "Error creating renderable from mesh: " << e.what() << std::endl;
            throw std::runtime_error("Failed to create renderable from mesh");
        }
    }

    std::shared_ptr<IRenderable> IRenderer::removeRenderable(uint64_t index)
    {
        for (auto& layer : _renderables)
        {
            if (layer.contains(index))
            {
                auto renderable = layer[index];
                layer.erase(index);
                // recycle the ID
                _freeIDs.push_back(index);
                return renderable;
            }
        }
        return nullptr;
    }

    void IRenderer::setVisible(uint64_t index, bool visible)
    {
        for (auto& layer : _renderables)
        {
            if (layer.contains(index))
            {
                layer[index]->visible = visible;
                return; // found and set visibility
            }
        }
    }

    void IRenderer::setWireframe(uint64_t index, bool wireframe)
    {
        for (auto& layer : _renderables)
        {
            if (layer.contains(index))
            {
                layer[index]->wireframe = wireframe;
                return; // found and set wireframe mode
            }
        }
    }
    const glm::mat4x4& IRenderer::modelMatrix(uint64_t index)
    {
        for (auto& layer : _renderables)
        {
            if (layer.contains(index))
            {
                return layer[index]->modelMatrix(); // return the model matrix of the renderable
            }
        }
        throw std::runtime_error("Renderable not found");
    }
    void IRenderer::modelMatrix(uint64_t index, const glm::mat4x4& matrix)
    {
        for (auto& layer : _renderables)
        {
            if (layer.contains(index))
            {
                layer[index]->modelMatrix(matrix); // set the model matrix of the renderable
                return; // found and set model matrix
            }
        }
    }

    // load the pipeline state objects from the PSOConfigs using the pso factory
    void IRenderer::loadPSOs(const std::unordered_map<std::string, const PSOConfig>& psoConfigs)
    {
        for (const auto& val : psoConfigs | std::views::values)
        {
            loadPSO(val);
        }
    }

    void IRenderer::loadPSO(const PSOConfig& config)
    {
        _pipelineStateObjects.emplace(
            config.name,
            _psoFactory->create(config)
        );
        // create materials
        for (const auto& materialInfo : config.materials)
        {
            if (materialInfo.frequency == MaterialFrequency::PerFrame)
            {
                _materialInfos[config.name].emplace_back(materialInfo);
                _materials[config.name].emplace_back(getDefaultBytes(materialInfo.type));
            }
        }
    }

    void IRenderer::setMaterial(const std::string& name, const std::vector<std::byte>& materialBytes, MaterialType type)
    {
        if (_materials.contains(name))
        {
            for (int i = 0; i < _materials.at(name).size(); i++)
            {
                if (_materialInfos[name][i].type == type)
                {
                    _materials[name][i] = materialBytes;
                    return;
                }
            }
        }
    }


    void IRenderer::setDirectionalLight(const DirectionalLight& light, int index)
    {
        if (index >= 0 && index < MAX_DIRECTIONAL_LIGHTS)
        {
            _lights.directionalLights[index] = light;
        }
        else
        {
            std::cerr << "Invalid directional light index: " << index << std::endl;
        }
    }
    void IRenderer::setPointLight(const PointLight& light, int index)
    {
        if (index >= 0 && index < MAX_POINT_LIGHTS)
        {
            _lights.pointLights[index] = light;
        }
        else
        {
            std::cerr << "Invalid point light index: " << index << std::endl;
        }
    }
    void IRenderer::clearLights()
    {
        _lights = Lights{}; // reset the lights structure
    }

    void IRenderer::setAmbientGlobalLight(const glm::vec4& color)
    {
        _lights.globalAmbientLightColor = color;
    }
    void IRenderer::setLights(const Lights& lights)
    {
        _lights = lights;
    }

    void IRenderer::setDebugUICallback(std::function<void()> callback)
    {
        _debugUICallback = std::move(callback);
    }

}
