//
// Created by Giovanni Bollati on 03/02/26.
//

#ifndef EVOLVING_PLANETS_COMPONENTS_HPP
#define EVOLVING_PLANETS_COMPONENTS_HPP
#include "glm/fwd.hpp"
#include "glm/gtc/quaternion.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/quaternion.hpp"
#include <typeindex>
#include <utility>
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>
#include <memory>

#include "BVH.hpp"
#include "Rendering/Material.hpp"

class Planet;
namespace Core {
    class Texture;
}


struct BVHRequestComponent {};
struct BVHComponent
{
    BVH bvh;
    std::vector<glm::vec3> vertices;
    std::vector<uint32_t> indices;
};
struct BVHMaterialComponent
{
    bool dirty = true;
};

struct PlanetConfigComponent {
    int nParallels = 15;
    int nMeridians = 15;
    float baseRadius = 250.0f;
    int nMutations = 0;
    int textureSize = 512;
    bool isDirty = true;
};

struct PlanetDataComponent {
    std::shared_ptr<Planet> planet;
};

struct PlanetInfoComponent {
    CompactPlanetInfoMaterial info;
};

using namespace Geometry;

// RENDERING
struct RenderableComponent
{
    uint64_t id;
};
struct MaterialComponent
{
    uint64_t id;
};
// if an entity has this component, and the entity has a component from which a renderable can created, such as a mesh, then
// a renderable and a material will be created, RenderableComponent and MaterialComponent will be added and
// the entity will be submitted to the render queue
struct RenderRequestComponent{};
struct RenderConfigComponent
{
    RenderConfigComponent() = delete;
    RenderConfigComponent(
        const std::string& psoName,
        bool castShadow = false,
        Rendering::RenderLayer layer = Rendering::RenderLayer::OPAQUE
        ) :
            psoName(psoName),
            layer(layer),
            visible(true),
            castShadow(castShadow)
    {}
    std::string psoName;
    Rendering::RenderLayer layer;
    bool visible;
    bool castShadow = false;
    bool wireframe = false;
    glm::ivec3 gridSize = glm::ivec3(0, 0, 0);
    glm::ivec3 threadgroupSize = glm::ivec3(0, 0, 0);
};

// MESH

// pointer to mesh
struct MeshComponent
{
    MeshComponent(const std::shared_ptr<Geometry::Mesh>& m, std::string  p = "procedural") : mesh(m), path(std::move(p)) {
        assert(mesh && "MeshComponent: mesh is null during component construction");
    }
    MeshComponent() = delete;

    std::shared_ptr<Geometry::Mesh> mesh{};
    std::string path;
};
struct MeshRequestComponent
{
    std::string path;
};

// TRANSFORM
struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    void rotate(float angle, glm::vec3 axis) { rotation = glm::rotate(rotation, glm::radians(angle), axis); }

    [[nodiscard]] glm::mat4 modelMatrix() const
    {
        glm::mat4 model = glm::translate(glm::mat4(1.0f), position);
        model = model * glm::toMat4(rotation);
        model = glm::scale(model, scale);
        return model;
    }
};

struct TexturesRequestComponent
{
    std::vector<std::string> paths;
    std::vector<TextureType> types;
};

struct TexturesComponent
{
    explicit TexturesComponent(const std::vector<std::shared_ptr<Texture>>& t) : textures(t)
    {
        for (auto& texture : textures)
        {
            assert(texture && "TexturesComponent: one of the textures is null during component construction");
        }
    }
    std::vector<std::shared_ptr<Texture>> textures = std::vector<std::shared_ptr<Texture>>();
};


// pointer to camera
struct CameraComponent
{
    CameraComponent(const std::shared_ptr<Camera>& c) : camera(c) {
        assert(camera && "CameraComponent: camera is null during component construction");
    }
    CameraComponent() = delete;
    std::shared_ptr<Camera> camera;
};

struct DirectionalLightComponent
{
    DirectionalLight light = DirectionalLight();
    float intensity = 1.0f;
    float distance = 250.0f; //distance from origin along light -dir light direction
    float nearPlane = 1.0f;
    float farPlane = 500.0f;
    float orthoSize = 250.0f; // size of the orthographic projection for shadow mapping
};

struct PointLightComponent
{
    PointLight light = PointLight();
    float intensity = 1.0f;
};

// world coordinated of MouseRay
struct MouseRay
{
    glm::vec3 origin = glm::vec3(0.0f, 0.0f, 0.0f);
    glm::vec3 direction = glm::vec3(1.0f, 1.0f, 1.0f);
};

struct MouseRayIntersectionComponent
{
    bool intersected = false;
    glm::vec3 intersection = glm::vec3(0.0f);
};

struct ViewportComponent
{
    bool letterbox = false;
    glm::vec4 normalizedViewport = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    glm::vec2 aspectRatio = glm::vec2(16.0f, 9.0f);

    std::pair<float, glm::vec4> getData(glm::vec2 drawableSize, bool normalized = true)
    {
        auto ar = aspectRatio[0] / aspectRatio[1];
        if (letterbox)
        {
            glm::vec4 pixelViewport = {drawableSize[0] * normalizedViewport[0], drawableSize[1] * normalizedViewport[1], drawableSize[0] * normalizedViewport[2], drawableSize[1] * normalizedViewport[3]};
            if (drawableSize[1] * ar > drawableSize[0])
            {
                // verticale troppo grande -> bande sopra e sotto
                auto w = drawableSize[0];
                auto h = drawableSize[0] / ar;
                auto x = 0.0f;
                auto y = (drawableSize[1] - h) /  2.0f;
                if (normalized)
                {
                    return {ar, glm::vec4(x, y / drawableSize[1], w / drawableSize[0], h / drawableSize[1])};
                }
                else
                {
                    return {ar, glm::vec4(x, y, w, h)};
                }
            }
            else
            {
                // orizzontale troppo grande -> bande sinistra e destra
                auto h = drawableSize[1];
                auto w = drawableSize[1] * ar;
                auto x = (drawableSize[0] - w) / 2.0f;
                auto y = 0.0f;
                if (normalized)
                {
                    return {ar, glm::vec4(x / drawableSize[0], y, w / drawableSize[0], h / drawableSize[1])};
                }
                else
                {
                    return {ar, glm::vec4(x, y, w, h)};
                }
            }
        }
        else
        {
            ar = (drawableSize[0] * normalizedViewport[2]) / (drawableSize[1] * normalizedViewport[3]);
            if (normalized)
            {
                return {ar, normalizedViewport};
            }
            else
            {
                return {ar, glm::vec4(normalizedViewport[0] * drawableSize[0], normalizedViewport[1] * drawableSize[1], normalizedViewport[2] * drawableSize[0], normalizedViewport[3] * drawableSize[1])};
            }
        }
    }
};

struct NameComponent
{
    explicit NameComponent(std::string n) : name(std::move(n)) {}
    NameComponent() = delete;
    std::string name;
};

struct PSOName
{
    explicit PSOName(std::string n) : name(std::move(n)) {}
    PSOName() = delete;
    std::string name;
};

struct RectMaterialComponent
{
    RectMaterial material = RectMaterial();
};
struct ViewportSizeMaterialComponent
{
    ViewportSize material = ViewportSize();
};

// MATERIAL COMPONENTS
struct TintMaterialComponent
{
    Tint material = Tint();
};

struct ShininessMaterialComponent
{
    float shininess = 1.0f;
};

struct BoundingSphereRequestComponent {};

struct BoundingSphereComponent
{
    BoundingSphereComponent() = delete;
    explicit BoundingSphereComponent(const glm::vec4& data)
    {
        center = {data.x, data.y, data.z};
        radius = data.w;
    }
    glm::vec3 center = glm::vec3(0.0f);
    float radius = 0.0f;
};

struct AmbientLightComponent
{
    glm::vec3 light = glm::vec3(0.0f);
};

struct WardMaterialComponent
{
    WardMaterialComponent() = default;
    float roughness = 0.5f;
    float metallic = 0.0f;
};


// PHYSICS
enum class ColliderType
{
    NONE,
    SPHERE,
    BOX,
    MESH
};

struct Collider
{
    ColliderType type = ColliderType::NONE;
};


struct RigidBodyComponent
{
    RigidBodyComponent(ColliderType type) : colliderType(type) {}
    ~RigidBodyComponent()
    {
        if (motionState)
        {
            delete motionState;
            motionState = nullptr;
        }
        if (btCollider)
        {
            delete btCollider;
            btCollider = nullptr;
        }
    }

    bool dirty = true;
    bool active = false;

    void setActive(bool a) { active = a; dirty = true;}
    // rigid body properties
    float mass = 1.0f;
    void setMass(float m) { mass = m; dirty = true;}
    float bounciness = 0.0f;
    void setBounciness(float b) { bounciness = b; dirty = true;}
    float friction = 0.3f;
    void setFriction(float f) { friction = f; dirty = true;}
    bool isKinematic = false;
    void setKinematic(bool k) { isKinematic = k; dirty = true;}

    // collider
    ColliderType colliderType = ColliderType::NONE;
    float radius = 1.0f;
    void setRadius(float r) { radius = r; dirty = true;}
    glm::vec3 boxHalfExtents = glm::vec3(1.0f);
    void setBoxHalfExtents(glm::vec3 h) { boxHalfExtents = h; dirty = true;}

    // bullet data structures
    btCollisionShape* btCollider = nullptr;
    btMotionState* motionState = nullptr;
    btRigidBody* body = nullptr;

    btVector3 localIntertia = btVector3(0.0f, 0.0f, 0.0f);

    bool teleportRequested = false;
    glm::vec3 teleportPos;
    glm::quat teleportRot;

    void teleport(glm::vec3 pos, glm::quat rot) {
        teleportPos = pos;
        teleportRot = rot;
        teleportRequested = true;
    }
};




#endif //EVOLVING_PLANETS_COMPONENTS_HPP