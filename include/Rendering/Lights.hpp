//
// Created by Giovanni Bollati on 24/06/25.
//

#ifndef LIGHTS_HPP
#define LIGHTS_HPP

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct DirectionalLight
{
    glm::vec4 direction; // additional w for padding
    glm::vec4 color = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // r, g, b // brightness of the light
};

struct PointLight {
    glm::vec4 position; // additional w for padding
    glm::vec4 color; // r, g, b// brightness of the light
};

struct Lights {
    glm::vec4 globalAmbientLightColor = glm::vec4{0.0f, 0.0f, 0.0f, 1.0f}; // default ambient light color
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    int numDirectionalLights = 0;
    int numPointLights = 0;
    int _padding[2] = {0, 0};
};

#endif //LIGHTS_HPP
