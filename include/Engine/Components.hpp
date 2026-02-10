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


struct RenderableComponent
{
    uint64_t id;
};


struct Transform
{
    glm::vec3 position = glm::vec3(0.0f);
    glm::quat rotation = glm::quat(1.0f, 0.0f, 0.0f, 0.0f);
    glm::vec3 scale = glm::vec3(1.0f);

    [[nodiscard]] glm::mat4 modelMatrix() const
    {
        return glm::toMat4(rotation) * glm::scale(glm::translate(glm::mat4(1.0f), position), scale);
    }
};

// pointer to mesh
struct MeshComponent
{
    MeshComponent(const std::shared_ptr<Mesh>& m, const std::string& p) : mesh(m), path(p) {
        assert(mesh && "MeshComponent: mesh is null during component construction");
    }
    MeshComponent() = delete;

    std::shared_ptr<Mesh> mesh;
    std::string path;
};

struct MeshRequestComponent
{
    std::string path;
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

struct RenderConfigComponent
{
    std::string psoName;
    Rendering::RenderLayer layer;
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
};

struct PointLightComponent
{
    PointLight light = PointLight();
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




#endif //EVOLVING_PLANETS_COMPONENTS_HPP