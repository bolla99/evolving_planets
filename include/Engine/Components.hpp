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

    void ui()
    {
        if (ImGui::CollapsingHeader("Transform"))
        {
            ImGui::InputFloat3("Position", glm::value_ptr(position));
            ImGui::InputFloat3("rotation", glm::value_ptr(rotation));
            ImGui::InputFloat3("scale", glm::value_ptr(scale));
        }
    }
};

// pointer to mesh
struct MeshComponent
{
    MeshComponent(const std::shared_ptr<Mesh>& m) : mesh(m) {
        assert(mesh && "MeshComponent: mesh is null during component construction");
    }
    MeshComponent() = delete;
    std::shared_ptr<Mesh> mesh;
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
    DirectionalLight light;

    void ui()
    {
        if (ImGui::CollapsingHeader("Directional Light"))
        {
            ImGui::InputFloat3("Color", glm::value_ptr(light.color));
            ImGui::InputFloat3("Base Direction", glm::value_ptr(light.direction));
        }
    }
};

struct PointLightComponent
{
    PointLight light;

    void ui()
    {
        if (ImGui::CollapsingHeader("Point Light"))
        {
            ImGui::InputFloat3("Color", glm::value_ptr(light.color));
        }
    }
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

struct NameComponent
{
    std::string name;
};

template <class T>
concept HasUI = requires(T& t)
{
    t.ui();
};

template <class T>
void DrawComponentUI(T& component)
{
    if constexpr (HasUI<T>)
        component.ui();
    //else
        //ImGui::Text("No ui() for this component type");
}



#endif //EVOLVING_PLANETS_COMPONENTS_HPP