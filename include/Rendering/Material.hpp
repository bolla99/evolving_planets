//
// Created by Giovanni Bollati on 30/01/26.
//

#ifndef EVOLVING_PLANETS_MATERIAL_HPP
#define EVOLVING_PLANETS_MATERIAL_HPP

#include "glm/vec4.hpp"

#include "glm/fwd.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <vector>

#include "BVH.hpp"
#include "Lights.hpp"
#include "glm/gtc/matrix_inverse.hpp"

enum MaterialType
{
    TINT,
    RECT,
    VIEWPORT_SIZE,
    MODEL_MATRIX,
    NORMAL_MATRIX,
    SHININESS,
    CAMERA_POSITION,
    VIEW_MATRIX,
    PROJECTION_MATRIX,
    VIEW_PROJECTION_MATRIX,
    LIGHTS,
    ROUGHNESS,
    METALLIC,
    SHADOW_DATA,
    POINT_SHADOW_DATA,
    DIRECTIONAL_LIGHT_INDEX,
    CAMERA_FAR_PLANE,
    INVERSE_VIEW_MATRIX,
    INVERSE_PROJECTION_MATRIX,
    PLANET_CP,
    PLANET_KNOTS_U,
    PLANET_KNOTS_V,
    PLANET_INFO,
    COMPACT_PLANET_INFO,
    PLANET_TEXTURE,
    PLANET_NORMAL_TEXTURE,
    BSPLINE_TEXTURE,
    ROCKY_NOISE_TEXTURE,
    BVH_NODES,
    BVH_PRIMITIVES,
};

enum MaterialStage
{
    Vertex,
    Fragment,
    Mesh,
    Object
};

struct MaterialInfo
{
    MaterialType type;
    MaterialStage stage;
    int bufferIndex;
};

struct BVHMaterial
{
    std::vector<BVHNode> data;
    std::vector<uint> primitives;
    std::vector<glm::vec3> vertices;
    std::vector<uint> indices;
};

// MATERIALS
struct Tint
{
    uint8_t addTint = 0;
    uint8_t _padding[15]{};
    glm::vec4 tintColor = glm::vec4(1.0f);
};

struct RectMaterial
{
    glm::vec4 rect = {0.0f, 0.0f, 100.0f, 100.0f};
};

struct ViewportSize
{
    float width = 100.0f;
    float height = 100.0f;
    // padding for 16byte alignment
    float padding1 = 0.0f;
    float padding2 = 0.0f;
};

struct ShadowData
{
    ShadowData()
    {
        for (auto& light : lightViewMatrix)
            light = glm::mat4(1.0f);
    }
    glm::mat4 lightViewMatrix[MAX_DIRECTIONAL_LIGHTS];
};

struct PointShadowData
{
    PointShadowData()
    {
        for (auto& matrix : matrices)
            matrix = glm::mat4(1.0f);
        for (auto& position : positions)
            position = glm::vec4(0.0f);
        for (auto& farPlane : farPlanes)
            farPlane = glm::vec4(0.0f);
    }
    glm::mat4 matrices[MAX_POINT_LIGHTS * 6];
    glm::vec4 positions[MAX_POINT_LIGHTS];
    glm::vec4 farPlanes[MAX_POINT_LIGHTS];
};

struct PlanetInfoMaterial {
    int degreeU = 0;
    int degreeV = 0;
    int nCP_U = 0;
    int nCP_V = 0;
    float planetRadius = 0.0f;
    int usePositionTexture = 0;
    int useNormalTexture = 0;
    int constantLOD = 0;
    int useConstantLOD = 0;
    int showMesh = 1;
    int isRocky = 0;
    int fractalOctaves = 16;
    float fractalIntensity = 1.0f;
    float fractalScale = 0.1f;
    int useRockyTexture = 0;
    int quadrantX = 0;
    int quadrantY = 0;
    int rockyResolution = 0;
};

struct CompactPlanetInfoMaterial {
    float planetRadius = 0.0f;
    float fractalIntensity = 15.0f;
    float fractalScale = 0.01f;
    int useConstantLOD = 0;
    int constantLOD = 0;
    int useHBAO = 0;
    int octaves = 12;
    float deltaMultiplier = 5.0f;
    float minDelta = 0.000004f;
    float maxDelta = 0.001f;
    int useRayTracingShadows = 1;
};

template <typename T>
std::vector<std::byte> getBytes(const T& material)
{
    auto bytes = std::vector<std::byte>(sizeof(T));
    std::memcpy(bytes.data(), &material, sizeof(T));
    return bytes;
}

// Support for vector serialization generally (if needed)
template <typename T>
std::vector<std::byte> getBytes(const std::vector<T>& vec)
{
    size_t size = vec.size() * sizeof(T);
    std::vector<std::byte> bytes(size);
    std::memcpy(bytes.data(), vec.data(), size);
    return bytes;
}

constexpr std::vector<std::byte> getDefaultBytes(MaterialType type)
{
    switch (type)
    {
    case TINT:
        return getBytes(Tint());
    case RECT:
        return getBytes(RectMaterial());
    case VIEWPORT_SIZE:
        return getBytes(ViewportSize());
    case MODEL_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case NORMAL_MATRIX:
        return getBytes(
        glm::inverseTranspose(glm::mat4(1.0f))
            );
    case SHININESS:
        return getBytes(1.0f);
    case CAMERA_POSITION:
        return getBytes(glm::vec4(0.0f));
    case VIEW_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case PROJECTION_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case VIEW_PROJECTION_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case LIGHTS:
        return getBytes(Lights());
    case ROUGHNESS:
        return getBytes(0.7f);
    case METALLIC:
        return getBytes(0.0f);
    case SHADOW_DATA:
        return getBytes(ShadowData());
    case POINT_SHADOW_DATA:
        return getBytes(PointShadowData());
    case DIRECTIONAL_LIGHT_INDEX:
        return getBytes(0);
    case CAMERA_FAR_PLANE:
        return getBytes(1000.0f);
    case INVERSE_PROJECTION_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case INVERSE_VIEW_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case PLANET_CP:
        return {};
    case PLANET_KNOTS_U:
        return {};
    case PLANET_KNOTS_V:
        return {};
    case PLANET_INFO:
        return getBytes(PlanetInfoMaterial{});
    case COMPACT_PLANET_INFO:
        return getBytes(CompactPlanetInfoMaterial{});
    case BSPLINE_TEXTURE:
        return {};
    case ROCKY_NOISE_TEXTURE:
        return {};
    case BVH_NODES:
        return {};
    case BVH_PRIMITIVES:
        return {};
    }
    throw std::runtime_error("Invalid material type");
}


#endif //EVOLVING_PLANETS_MATERIAL_HPP
