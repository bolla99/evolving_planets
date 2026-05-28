//
// Created by Giovanni Bollati on 07/02/26.
//

#ifndef EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP
#define EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP

#include <functional>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include <Engine/ECS/World.hpp>
#include <Engine/ECS/Components.hpp>
#include <Engine/ECS/Systems.hpp>
#include "Planet.hpp"

#include <imgui.h>

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
        /*
        ci.registerComponent<NameComponent>([](World& world, Context& ctx, EntityID entity, NameComponent& name)
        {
            if (ImGui::CollapsingHeader("Name"))
            {
                ImGui::Text("Name: %s", name.name.c_str());
            }
        });
        */
        ci.registerComponent<Transform>([](World& world, Context& ctx, EntityID entity, Transform& transform)
        {
            if (ImGui::CollapsingHeader("Transform"))
            {
                auto positionChanged = ImGui::InputFloat3("Position", glm::value_ptr(transform.position));
                auto eulers = glm::vec3(glm::degrees(glm::eulerAngles(transform.rotation)));
                auto rotationChanged = ImGui::InputFloat3("Rotation", glm::value_ptr(eulers));
                transform.rotation = glm::quat(glm::radians(eulers));
                auto scaleChanged = ImGui::InputFloat3("Scale", glm::value_ptr(transform.scale));
                if (positionChanged || rotationChanged || scaleChanged)
                {
                    if (world.hasComponent<RigidBodyComponent>(entity))
                    {
                        auto& rb = world.getComponent<RigidBodyComponent>(entity);
                        rb.teleport(transform.position, transform.rotation);
                    }
                }
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
                ImGui::SliderFloat("Intensity: ", &plc.intensity, 0.0f, 10.0f);
            }
        });
        ci.registerComponent<DirectionalLightComponent>([](World& world, Context& ctx, EntityID entity, DirectionalLightComponent& dlc)
        {
            if (ImGui::CollapsingHeader("Directional Light"))
            {
                ImGui::InputFloat3("Color", glm::value_ptr(dlc.light.color));
                ImGui::InputFloat3("Base Direction", glm::value_ptr(dlc.light.direction));
                ImGui::SliderFloat("Intensity: ", &dlc.intensity, 0.0f, 10.0f);
                ImGui::Text("Shadow Data");
                ImGui::InputFloat("Distance", &dlc.distance);
                ImGui::InputFloat("Near Plane", &dlc.nearPlane);
                ImGui::InputFloat("Far Plane", &dlc.farPlane);
                ImGui::InputFloat("Ortho Size", &dlc.orthoSize);
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
                static int currentPSO = 0;
                auto psoStrings = std::vector<const char*>();
                for (const auto& [name, config] : Rendering::psoConfigs)
                {
                    if (renderConf.psoName == name) currentPSO = psoStrings.size();
                    psoStrings.push_back(const_cast<char*>(name.c_str()));
                }
                ImGui::Combo("pso", &currentPSO, psoStrings.data(), psoStrings.size());
                renderConf.psoName = psoStrings[currentPSO];
                ImGui::Text("PSO: %s", renderConf.psoName.c_str());
                ImGui::Text("Layer: %d", renderConf.layer);
                ImGui::Checkbox("Visible", &renderConf.visible);
                ImGui::Checkbox("Wireframe", &renderConf.wireframe);
                ImGui::Checkbox("Cast Shadow", &renderConf.castShadow);
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
        ci.registerComponent<MaterialComponent>([](World& world, Context& ctx, EntityID entity, MaterialComponent& material)
        {
            if (ImGui::CollapsingHeader("Material"))
            {
                ImGui::Text("Material ID: %d", material.id);
            }
        });

        ci.registerComponent<ShininessMaterialComponent>([](World& world, Context& ctx, EntityID entity, ShininessMaterialComponent& material)
        {
            if (ImGui::CollapsingHeader("Shininess Material"))
            {
                ImGui::InputFloat("Shininess", &material.shininess);
            }
        });

        ci.registerComponent<BoundingSphereComponent>([](World& world, Context& ctx, EntityID entity, BoundingSphereComponent& sphere)
        {
            if (ImGui::CollapsingHeader("Bounding Sphere"))
            {
                ImGui::Text("Bounding Sphere Center: %f, %f, %f", sphere.center.x, sphere.center.y, sphere.center.z);
                ImGui::Text("Bounding Sphere Radius: %f", sphere.radius);
            }
        });

        ci.registerComponent<AmbientLightComponent>([](World& world, Context& ctx, EntityID entity, AmbientLightComponent& alc)
        {
            if (ImGui::CollapsingHeader("Ambient Light"))
            {
                ImGui::InputFloat3("Color", glm::value_ptr(alc.light));
            }
        });

        ci.registerComponent<WardMaterialComponent>([](World& world, Context& ctx, EntityID entity, WardMaterialComponent& material)
        {
            if (ImGui::CollapsingHeader("Ward Material"))
            {
                ImGui::SliderFloat("roughness", &material.roughness, 0.0f, 1.0f);
                ImGui::SliderFloat("metallic", &material.metallic, 0.0f, 1.0f);
            }
        });
        ci.registerComponent<RigidBodyComponent>([](World& world, Context& ctx, EntityID entity, RigidBodyComponent& rb)
        {
            if (ImGui::CollapsingHeader("Rigid Body"))
            {
                ImGui::Text("Mass: %f", rb.mass);
                auto active = rb.active;
                if (ImGui::Checkbox("Active", &active))
                {
                    rb.setActive(active);
                }
                ImGui::Text("Is Kinematic: %s", rb.isKinematic ? "true" : "false");
            }
        });

        ci.registerComponent<PlanetConfigComponent>([](World& world, Context& ctx, EntityID entity, PlanetConfigComponent& config)
        {
            if (ImGui::CollapsingHeader("Planet Config"))
            {
                if (ImGui::InputInt("Parallels", &config.nParallels)) {}
                if (ImGui::InputInt("Meridians", &config.nMeridians)) {}
                if (ImGui::InputFloat("Base Radius", &config.baseRadius)) {}
                if (ImGui::InputInt("Mutations", &config.nMutations)) {}
                if (ImGui::InputInt("Texture Size", &config.textureSize)) {}
                if (ImGui::Button("Regenerate")) config.isDirty = true;
            }
        });

        ci.registerComponent<PlanetDataComponent>([](World& world, Context& ctx, EntityID entity, PlanetDataComponent& data)
        {
            if (ImGui::CollapsingHeader("Planet Data"))
            {
                if (data.planet) {
                    ImGui::Text("Degree U: %d", data.planet->degreeU());
                    ImGui::Text("Degree V: %d", data.planet->degreeV());
                    ImGui::Text("CP Count: %zu", data.planet->controlPoints().size());
                } else {
                    ImGui::Text("No Planet Instance");
                }
            }
        });

        ci.registerComponent<PlanetInfoComponent>([](World& world, Context& ctx, EntityID entity, PlanetInfoComponent& matComp)
        {
            if (ImGui::CollapsingHeader("Planet Material"))
            {
                auto& mat = matComp.info;
                ImGui::Text("Current Radius: %f", mat.planetRadius);
                ImGui::InputFloat("Fractal Intensity", &mat.fractalIntensity);
                ImGui::InputFloat("Fractal Scale", &mat.fractalScale);
                bool useConstantLOD = mat.useConstantLOD != 0;
                if (ImGui::Checkbox("Use Constant LOD", &useConstantLOD)) {
                    mat.useConstantLOD = useConstantLOD ? 1 : 0;
                }
                ImGui::Text("Current LOD: %d", mat.constantLOD);
                bool useHBAO = mat.useHBAO != 0;
                if (ImGui::Checkbox("Use HBAO", &useHBAO)) {
                    mat.useHBAO = useHBAO ? 1 : 0;
                }
                ImGui::InputInt("Octaves", &mat.octaves);
                ImGui::InputFloat("Delta Multiplier", &mat.deltaMultiplier);
                ImGui::InputFloat("Min Delta", &mat.minDelta, 0, 0, "%.10f");
                ImGui::InputFloat("Max Delta", &mat.maxDelta);

                bool useRayTracingShadows = mat.useRayTracingShadows != 0;
                if (ImGui::Checkbox("Use Ray Tracing Shadows", &useRayTracingShadows)) {
                    mat.useRayTracingShadows = useRayTracingShadows ? 1 : 0;
                }
            }
        });

        ci.registerComponent<TexturesComponent>([](World& world, Context& ctx, EntityID entity, TexturesComponent& matComp)
        {
            if (ImGui::CollapsingHeader("Textures Component"))
            {
                ImGui::Text("Number of Textures: %zu", matComp.textures.size());
            }
        });

        ci.registerComponent<BVHComponent>([](World& world, Context& ctx, EntityID entity, BVHComponent& matComp)
        {
            if (ImGui::CollapsingHeader("BVH"))
            {
                //ImGui::Text("BVH Nodes: %zu", matComp.bvh.);
                ImGui::Text("Vertices: %zu", matComp.vertices.size());
                ImGui::Text("Indices: %zu", matComp.indices.size());
            }
        });
        return ci;
    }

private:
    std::vector<std::type_index> _order;
    std::unordered_map<std::type_index, Drawer> _componentInspectors;
};

#endif //EVOLVING_PLANETS_COMPONENTSINSPECTOR_HPP

