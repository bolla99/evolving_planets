//
// Created by Giovanni Bollati on 21/06/25.
//

#include <iostream>
#include <Rendering/IRenderer.hpp>
#include <Mesh.hpp>
#include <ranges>

namespace Rendering
{
    void IRenderer::addRenderable(const Mesh& mesh, const PSOConfig& psoConfig)
    {
        if (_pipelineStateObjects.find(psoConfig.name) == _pipelineStateObjects.end())
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
            auto renderable = _renderableFactory->fromMesh(mesh, _pipelineStateObjects.at(psoConfig.name));
            if (!renderable)
            {
                throw std::runtime_error("Failed to create renderable from mesh");
            }
            _renderables.push_back(renderable);
        } catch (const std::exception& e)
        {
            std::cerr << "Error creating renderable from mesh: " << e.what() << std::endl;
            throw std::runtime_error("Failed to create renderable from mesh");
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
}
