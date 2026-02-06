//
// Created by Giovanni Bollati on 24/06/25.
//



#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#include <glm/glm.hpp>

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct DirectionalLight
{
    glm::vec4 direction = glm::vec4{1.0f, 1.0f, 1.0f, 0.0f}; // additional w for padding
    glm::vec4 color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // r, g, b // brightness of the light
};

struct PointLight {
    glm::vec4 position = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // additional w for padding
    glm::vec4 color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // r, g, b// brightness of the light
};

struct Lights {
    glm::vec4 globalAmbientLightColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // default ambient light color
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
};

/*
class LightsManager
{
public:
    uint8_t addDirectionalLight(DirectionalLight light)
    {
        if (_numDirectionalLights >= MAX_DIRECTIONAL_LIGHTS)
        {
            throw std::runtime_error("Maximum number of directional lights reached.");
        }
        auto newID = _freeDirectionalLightIDs.empty() ? _nextDirectionalLightID++ : _freeDirectionalLightIDs.back();
        _directionalLights.insert({newID, light});
        // update lights structure
        _lights.directionalLights[_numDirectionalLights] = light;
        _lights.numDirectionalLights++;
        // increase number
        _numDirectionalLights++;
        return newID;
    }
    uint8_t addPointLight(PointLight light)
    {
        if (_numPointLights >= MAX_POINT_LIGHTS)
        {
            throw std::runtime_error("Maximum number of point lights reached.");
        }
        auto newID = _freePointLightsIDs.empty() ? _nextPointLightID++ : _freePointLightsIDs.back();
        _pointLights.insert({newID, light});
        // update lights structure
        _lights.pointLights[_numPointLights] = light;
        _lights.numPointLights++;
        // increase number
        _numPointLights++;
        return newID;
    }
    DirectionalLight getDirectionalLightID(uint8_t index)
    {
        if (!_directionalLights.contains(index)) throw std::runtime_error("Invalid directional light ID.");
        return _directionalLights.at(index);
    }
    PointLight getPointLightID(uint8_t index)
    {
        if (!_pointLights.contains(index)) throw std::runtime_error("Invalid point light ID.");
        return _pointLights.at(index);
    }

    bool setDirectionalLight(uint8_t id, DirectionalLight light)
    {
        if (!_directionalLights.contains(id)) return false;
        _directionalLights.at(id) = light;
    }
    bool setPointLight(uint8_t id, PointLight light)
    {
        if (!_pointLights.contains(id)) return false;
        _pointLights.at(id) = light;
    }

    void removeDirectionalLight(uint8_t id)
    {
        if (!_directionalLights.contains(id)) throw std::runtime_error("Invalid directional light ID.");
        _directionalLights.erase(id);
        _freeDirectionalLightIDs.push_back(id);
        _numDirectionalLights--;
        // rebuild lights structure
        _lights.numDirectionalLights = 0;
        for (const auto& [lightID, light] : _directionalLights)
        {
            _lights.directionalLights[_lights.numDirectionalLights] = light;
            _lights.numDirectionalLights++;
        }
    }

    LightsManager() = default;

private:
    Lights _lights;
    int _numDirectionalLights = 0;
    int _numPointLights = 0;
    std::unordered_map<uint8_t, DirectionalLight> _directionalLights;
    std::unordered_map<uint8_t, PointLight> _pointLights;
    uint8_t _nextDirectionalLightID = 0;
    uint8_t _nextPointLightID = 0;
    std::vector<uint8_t> _freeDirectionalLightIDs;
    std::vector<uint8_t> _freePointLightsIDs;
};
*/

#endif //LIGHTS_HPP
