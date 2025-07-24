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
        if (_pipelineStateObjects.contains(psoConfig.name))
        {
            try
            {
                _pipelineStateObjects.emplace(
                    psoConfig.name,
                    _psoFactory->create(psoConfig)
                );
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
    }

    void IRenderer::addDirectionalLight(const DirectionalLight& light)
    {
        if (_lights.numDirectionalLights < MAX_DIRECTIONAL_LIGHTS)
        {
            _lights.directionalLights[_lights.numDirectionalLights++] = light;
        }
        else
        {
            std::cerr << "Maximum number of directional lights reached: " << MAX_DIRECTIONAL_LIGHTS << std::endl;
        }
    }
    void IRenderer::addPointLight(const PointLight& light)
    {
        if (_lights.numPointLights < MAX_POINT_LIGHTS)
        {
            _lights.pointLights[_lights.numPointLights++] = light;
        }
        else
        {
            std::cerr << "Maximum number of point lights reached: " << MAX_POINT_LIGHTS << std::endl;
        }
    }
    void IRenderer::clearLights()
    {
        _lights = Lights{}; // reset the lights structure
    }
    void IRenderer::removeLastDirectionalLight()
    {
        if (_lights.numDirectionalLights > 0)
        {
            --_lights.numDirectionalLights;
            _lights.directionalLights[_lights.numDirectionalLights] = DirectionalLight{}; // reset the last light
        }
        else
        {
            std::cerr << "No directional lights to remove." << std::endl;
        }
    }
    void IRenderer::removeLastPointLight()
    {
        if (_lights.numPointLights > 0)
        {
            --_lights.numPointLights;
            _lights.pointLights[_lights.numPointLights] = PointLight{}; // reset the last light
        }
        else
        {
            std::cerr << "No point lights to remove." << std::endl;
        }

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
