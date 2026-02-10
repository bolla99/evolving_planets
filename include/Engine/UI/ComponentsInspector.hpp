//
// Created by Giovanni Bollati on 07/02/26.
//

#ifndef EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP
#define EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <Engine/World.hpp>
#include <Engine/Components.hpp>
#include <Engine/Systems.hpp>

struct ComponentsInspector
{
    using EntityID = uint64_t;
    using Drawer = std::function<void(World&, Context&, EntityID)>;

    // draw entity inspector
    void entityInspector(World& world, Context& ctx, EntityID entity) const
    {
        // follow order
        for (const auto& type : _order)
        {
            if (_componentInspectors.contains(type))
            {
                _componentInspectors.at(type)(world, ctx, entity);
            }
        }
    }

    template <class T>
    void registerComponent(Drawer inspector)
    {
        const auto ti = std::type_index(typeid(T));
        if (!_componentInspectors.contains(ti))
        {
            _order.push_back(ti);
        }
        _componentInspectors[ti] = std::move(inspector);
    }

    // Registrazione "high-level": fornisci una funzione tipizzata sul componente.
    // Il wrapper generato controlla hasComponent<T> e poi chiama la callback con T&.
    template <class T>
    void registerComponent(std::function<void(World&, Context&, EntityID, T&)> inspector)
    {
        registerComponent<T>(
            [inspector = std::move(inspector)](World& world, Context& ctx, EntityID entity)
            {
                if (!world.hasComponent<T>(entity)) return;
                auto& component = world.getComponent<T>(entity);
                inspector(world, ctx, entity, component);
            }
        );
    }

    static ComponentsInspector factory()
    {
        auto ci = ComponentsInspector();
        ci.registerComponent<NameComponent>([](World& world, Context& ctx, EntityID entity, NameComponent& name)
        {
            if (!ImGui::CollapsingHeader("Name"))
            {
                ImGui::Text("Name: %s", name.name.c_str());
            }
        });
        ci.registerComponent<Transform>([](World& world, Context& ctx, EntityID entity, Transform& transform)
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                ImGui::InputFloat3("Position", glm::value_ptr(transform.position));
                ImGui::InputFloat3("Rotation", glm::value_ptr(transform.rotation));
                ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale));
            }
        });
        ci.registerComponent<TintMaterialComponent>([](World& world, Context& ctx, EntityID entity, TintMaterialComponent& tint)
        {
            if (ImGui::CollapsingHeader("Tint Material"))
            {
                auto& material = tint.material;
                bool active = material.addTint > 0 ? true : false;
                ImGui::Checkbox("Add Tint", &active);
                material.addTint = active ? 1.0f : 0.0f;
                ImGui::ColorEdit3("Tint", glm::value_ptr(material.tintColor));
            }
        });
        ci.registerComponent<PointLightComponent>([](World& world, Context& ctx, EntityID entity, PointLightComponent& plc)
        {
            if (ImGui::CollapsingHeader("Point Light"))
            {
                ImGui::InputFloat3("Color", glm::value_ptr(plc.light.color));
            }
        });
        ci.registerComponent<DirectionalLightComponent>([](World& world, Context& ctx, EntityID entity, DirectionalLightComponent& dlc)
        {
            if (ImGui::CollapsingHeader("Directional Light"))
            {
                ImGui::InputFloat3("Color", glm::value_ptr(dlc.light.color));
                ImGui::InputFloat3("Base Direction", glm::value_ptr(dlc.light.direction));
            }
        });
        ci.registerComponent<MeshComponent>([](World& world, Context& ctx, EntityID entity, MeshComponent& mesh)
        {
            if (ImGui::CollapsingHeader("Mesh"))
            {
                ImGui::Text("Mesh: %s", mesh.path.c_str());
                ImGui::Text(mesh.mesh->info().c_str());
            }
        });
        ci.registerComponent<RenderConfigComponent>([](World& world, Context& ctx, EntityID entity, RenderConfigComponent& renderConf)
        {
            if (ImGui::CollapsingHeader("Render Config"))
            {
                ImGui::Text("PSO: %s", renderConf.psoName.c_str());
                ImGui::Text("Layer: %d", renderConf.layer);
            }
        });
        ci.registerComponent<ViewportComponent>([](World& world, Context& ctx, EntityID entity, ViewportComponent& viewport)
        {
            if (ImGui::CollapsingHeader("Viewport"))
            {
                ImGui::Checkbox("Letterbox", &viewport.letterbox);
                ImGui::InputFloat4("Normalized Viewport", glm::value_ptr(viewport.normalizedViewport));
                ImGui::InputFloat2("Aspect Ratio", glm::value_ptr(viewport.aspectRatio));
            }
        });
        ci.registerComponent<MouseRay>([](World& world, Context& ctx, EntityID entity, MouseRay& ray)
        {
            if (ImGui::CollapsingHeader("MouseRay"))
            {
                ImGui::Text("Mouse Ray Origin: %f, %f, %f", ray.origin.x, ray.origin.y, ray.origin.z);
                ImGui::Text("Mouse Ray Direction: %f, %f, %f", ray.direction.x, ray.direction.y, ray.direction.z);
            }
        });
        ci.registerComponent<MouseRayIntersectionComponent>([](World& world, Context& ctx, EntityID entity, MouseRayIntersectionComponent& intersection)
        {
            if (ImGui::CollapsingHeader("Mouse Ray Intersection"))
            {
                ImGui::Text("Intersected: %s", intersection.intersected ? "true" : "false");
            }
        });
        ci.registerComponent<CameraComponent>([](World& world, Context& ctx, EntityID entity, CameraComponent& camera)
        {
            if (ImGui::CollapsingHeader("Camera"))
            {
                auto cam = camera.camera;
                ImGui::Text("Camera Position: %f, %f, %f", cam->position.x, cam->position.y, cam->position.z);
                ImGui::InputFloat("Near Plane", &cam->nearPlane);
                ImGui::InputFloat("Far Plane", &cam->farPlane);
                ImGui::InputFloat("Field of View", &cam->fov);
            }
        });
        ci.registerComponent<MeshRequestComponent>([](World& world, Context& ctx, EntityID entity, MeshRequestComponent& mesh)
        {
            if (ImGui::CollapsingHeader("Mesh Request"))
            {
                ImGui::Text("Mesh Request: %s", mesh.path.c_str());
            }
        });
        ci.registerComponent<RectMaterialComponent>([](World& world, Context& ctx, EntityID entity, RectMaterialComponent& rectComponent)
        {
            if (ImGui::CollapsingHeader("Rect Material"))
            {
                auto& [rect] = rectComponent.material;
                ImGui::InputFloat4("Color", glm::value_ptr(rect));
            }
        });
        ci.registerComponent<RenderableComponent>([](World& world, Context& ctx, EntityID entity, RenderableComponent& renderable)
        {
            if (ImGui::CollapsingHeader("Renderable"))
            {
                ImGui::Text("Renderable ID: %d", renderable.id);
            }
        });
        ci.registerComponent<ViewportSizeMaterialComponent>([](World& world, Context& ctx, EntityID entity, ViewportSizeMaterialComponent& viewportSize)
        {
            if (ImGui::CollapsingHeader("Viewport Size Material"))
            {
                ImGui::Text("Viewport Width: %f", viewportSize.material.width);
                ImGui::Text("Viewport Height: %f", viewportSize.material.height);
            }
        });
        return ci;
    }

private:
    std::vector<std::type_index> _order;
    std::unordered_map<std::type_index, Drawer> _componentInspectors;
};

#endif //EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP

