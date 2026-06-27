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
    JITTERED_VIEW_PROJECTION_MATRIX,
    PREVIOUS_VIEW_PROJECTION_MATRIX,
    LIGHTS,
    ROUGHNESS,
    METALLIC,
    SHADOW_DATA,
    POINT_SHADOW_DATA,
    DIRECTIONAL_LIGHT_INDEX,
    CAMERA_FAR_PLANE,
    INVERSE_VIEW_MATRIX,
    INVERSE_PROJECTION_MATRIX,
    INVERSE_VIEW_PROJECTION_MATRIX,
    JITTERED_INVERSE_PROJECTION_MATRIX,
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
    BVH_INFO,
    JITTER,
    TAAScaling,
    SUN_DIRECTION,
    UNLIT_COLOR,
    BILLBOARD_DATA,
    WARD_ALPHA,
    CAMERA_PLANES,
    PARTICLE_SOFTNESS,
    POTENTIAL_OCTREE,
    POTENTIAL_OCTREE_INFO,
    POTENTIAL_SAMPLING_INFO,
    SUN_COLOR,
    TIME,
    THURST,
    HEAT,
    PREVIOUS_MODEL_MATRIX,
    ATMOSPHERE_SETTINGS
};

enum MaterialStage
{
    Vertex,
    Fragment,
    Mesh,
    Object
};

struct AtmosphereSettings
{
    int SAMPLES = 16;
    int _padding[3];
    int SUN_SAMPLES = 8;
    int _padding2[3];
    bool jitter = true;
    int _padding3[3];
    bool useBakedLightTransmittance = false;
};


struct MaterialInfo
{
    MaterialType type;
    MaterialStage stage;
    int bufferIndex;
};

struct PotentialSamplingInfo
{
    glm::vec4 min = glm::vec4(0.0f);
    float edge = 0.0f;
    float _padding1[3];
    float nonZeroDensityRadius = 0.0f;
    float _padding2[3];
};

struct PotentialOctreeInfo
{
    glm::vec4 minBounds = glm::vec4(0.0f);
    int size = 0;
    float _padding1[3];
    float edge = 0.0f;
    float _padding2[3];
    float multiplier = 1.0f;
};

struct BVHInfo
{
    int nodesSize = 0;
    float _padding1[3];
    int primitivesSize = 0;
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

struct BillBoardMaterial
{
    glm::vec4 position = glm::vec4(0.0f);
    glm::vec4 color = glm::vec4(1.0f);
    float size = 2.0f;
    int useDepth = 0;
    float depth = 0.01f;
    float radius = 0.01f;
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
    float fractalIntensity = 20.0f;
    float fractalScale = 0.01f;
    int useConstantLOD = 0;
    int constantLOD = 0;
    int octaves = 14;
    float deltaMultiplier = 5.0f;
    float minDelta = 0.000003f;
    float maxDelta = 0.00001f;
    int useRayTracingShadows = 0;
    int useSkirts = 1;
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
    case JITTERED_VIEW_PROJECTION_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case PREVIOUS_VIEW_PROJECTION_MATRIX:
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
    case JITTERED_INVERSE_PROJECTION_MATRIX:
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
    case JITTER:
        return getBytes(glm::vec2(0.0f));
    case INVERSE_VIEW_PROJECTION_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case TAAScaling:
        return getBytes(1.0f);
    case SUN_DIRECTION:
        return getBytes(glm::vec4(0.0f));
    case UNLIT_COLOR:
        return getBytes(glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
    case BILLBOARD_DATA:
        return getBytes(BillBoardMaterial{});
    case WARD_ALPHA:
        return getBytes(1.0f);
    case CAMERA_PLANES:
        return getBytes(glm::vec2(1.0f));
    case PARTICLE_SOFTNESS:
        return getBytes(0.0f);
    case POTENTIAL_OCTREE:
        return {};
    case POTENTIAL_OCTREE_INFO:
        return getBytes(PotentialOctreeInfo{});
    case BVH_INFO:
        return getBytes(BVHInfo{});
    case POTENTIAL_SAMPLING_INFO:
        return getBytes(PotentialSamplingInfo{});
    case SUN_COLOR:
        return getBytes(glm::vec3(1.0f));
    case TIME:
        return getBytes(0.0f);
    case THURST:
        return getBytes(0.0f);
    case HEAT:
        return getBytes(0.0f);
    case PREVIOUS_MODEL_MATRIX:
        return getBytes(glm::mat4(1.0f));
    case ATMOSPHERE_SETTINGS:
        return getBytes(AtmosphereSettings{});
    }
    throw std::runtime_error("Invalid material type");
}


#endif //EVOLVING_PLANETS_MATERIAL_HPP
