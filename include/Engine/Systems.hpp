//
// Created by Giovanni Bollati on 03/02/26.
//

#ifndef EVOLVING_PLANETS_SYSTEMS_HPP
#define EVOLVING_PLANETS_SYSTEMS_HPP

#include "Engine/World.hpp"
#include "Rendering/IRenderer.hpp"
#include "Engine/Components.hpp"
#include "App.hpp"

struct Context
{
    Rendering::IRenderer* renderer;
    SDL_Window* window;

    bool isValid() const
    {
        return renderer != nullptr && window != nullptr;
    }
};

class ISystem
{
public:
    virtual void update(World& world, const Context& ctx, float dt) = 0;
    virtual ~ISystem() = default;
};

// this system takes entities that has a transform and a renderable id, and update the renderable model matrix
// from their transform
class TransformSystem : public ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<RenderableComponent, Transform>();
        for (auto& entity : entities)
        {
            std::cout << "Entity: " << entity << std::endl;
            auto rc = world.getComponent<RenderableComponent>(entity);
            auto transform = world.getComponent<Transform>(entity);
            ctx.renderer->modelMatrix(rc.id, transform.modelMatrix());
        }
    }
};

class MouseRaySystem : public ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        if (!ctx.isValid()) throw std::runtime_error("Invalid context");
        auto cameras = world.query<CameraComponent>();
        if (cameras.empty()) return;
        auto activeCamera = cameras[0];
        auto activeCameraComponent = world.getComponent<CameraComponent>(activeCamera);
        auto mouseRays = world.query<MouseRay>();
        if (mouseRays.empty()) return;
        auto& mouseRay = world.getComponent<MouseRay>(mouseRays[0]);
        auto mr = App::mouseRay(activeCameraComponent.camera->getViewMatrix(), ctx.window, ctx.renderer);
        mouseRay.origin = mr[0];
        mouseRay.direction = mr[1];
    }

};

class RenderRegistrationSystem : ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<MeshComponent, RenderConfigComponent>();
        for (auto& entity : entities)
        {
            if (!world.hasComponent<RenderableComponent>(entity))
            {
                auto& meshComp = world.getComponent<MeshComponent>(entity);
                auto& renderConfig = world.getComponent<RenderConfigComponent>(entity);
                auto renderableID = ctx.renderer->addRenderable(*meshComp.mesh, Rendering::psoConfigs.at(renderConfig.psoName), {}, renderConfig.layer);
                world.addComponent<RenderableComponent>(entity, {renderableID});
            }
        }
    }
};

class SetLightsSystem : ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        auto directionalLights = world.query<DirectionalLightComponent>();
        auto pointLights = world.query<PointLightComponent>();
        auto lights = Lights();
        auto directionalLightsSize = std::min(MAX_DIRECTIONAL_LIGHTS, static_cast<int>(directionalLights.size()));
        auto pointLightsSize = std::min(MAX_POINT_LIGHTS, static_cast<int>(pointLights.size()));
        for (int i = 0; i < directionalLightsSize; i++)
        {
            lights.directionalLights[i] = world.getComponent<DirectionalLightComponent>(directionalLights[i]).light;
        }
        for (int i = 0; i < pointLightsSize; i++)
        {
            lights.pointLights[i] = world.getComponent<PointLightComponent>(pointLights[i]).light;
        }
        ctx.renderer->setLights(lights);
    }
};

class MouseIntersectionSystem : public ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        auto mouseRays = world.query<MouseRay>();
        if (mouseRays.empty()) return;
        auto& mouseRay = world.getComponent<MouseRay>(mouseRays[0]);

        auto intersectableEntities = world.query<MeshComponent, Transform, MouseRayIntersectionComponent>();
        for (auto& entity : intersectableEntities)
        {
            auto& meshComp = world.getComponent<MeshComponent>(entity);
            if (!meshComp.mesh) continue;
            auto& transform = world.getComponent<Transform>(entity);
            auto& intersectionComp = world.getComponent<MouseRayIntersectionComponent>(entity);
            intersectionComp.intersected = false;

            auto originLocal = glm::inverse(transform.modelMatrix()) * glm::vec4(mouseRay.origin, 1.0f);
            auto directionLocal = glm::inverse(transform.modelMatrix()) * glm::vec4(mouseRay.direction, 0.0f);

            auto intersection = meshComp.mesh->rayIntersection(
                glm::vec3(originLocal.x, originLocal.y, originLocal.z),
                glm::vec3(directionLocal.x, directionLocal.y, directionLocal.z)
            );

            intersectionComp.intersected = intersection.first;
            intersectionComp.intersection = intersection.second;
        }
    }
};

class UpdateLightsWithTransformSystem : ISystem
{
public:
    void update(World& world, const Context& ctx, float dt) override
    {
        // set point lights
        auto pointLights = world.query<PointLightComponent, Transform>();
        for (auto e : pointLights)
        {
            auto& light = world.getComponent<PointLightComponent>(e).light;
            auto transform = world.getComponent<Transform>(e);
            light.position = glm::vec4(transform.position, 1.0f);
        }

        // rotate directional lights
        auto directionalLights = world.query<DirectionalLightComponent, Transform>();
        for (auto e : directionalLights)
        {
            auto& light = world.getComponent<DirectionalLightComponent>(e).light;
            auto transform = world.getComponent<Transform>(e);
            light.direction = glm::normalize(transform.rotation * light.direction);
        }

    }

};

#endif //EVOLVING_PLANETS_SYSTEMS_HPP