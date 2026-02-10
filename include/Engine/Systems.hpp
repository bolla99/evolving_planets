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
    AssetManager* assetManager;

    [[nodiscard]] bool isValid() const
    {
        return renderer != nullptr && window != nullptr && assetManager != nullptr;
    }
};

struct ISystem
{
    virtual void update(World& world, const Context& ctx, float dt) = 0;
    virtual ~ISystem() = default;
};

// this system takes entities that has a transform and a renderable id, and update the renderable model matrix
// from their transform
struct TransformSystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<RenderableComponent, Transform>();
        for (auto& entity : entities)
        {
            auto rc = world.getComponent<RenderableComponent>(entity);
            auto transform = world.getComponent<Transform>(entity);
            ctx.renderer->modelMatrix(rc.id, transform.modelMatrix());
        }
    }
};

struct MouseRaySystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        if (!ctx.isValid()) throw std::runtime_error("Invalid context");
        auto cameras = world.query<CameraComponent>();
        if (cameras.empty()) return;
        auto activeCamera = cameras[0];
        auto activeCameraComponent = world.getComponent<CameraComponent>(activeCamera);
        auto camera = activeCameraComponent.camera;
        auto mouseRays = world.query<MouseRay>();
        if (mouseRays.empty()) return;
        auto& mouseRay = world.getComponent<MouseRay>(mouseRays[0]);
        auto viewports = world.query<ViewportComponent>();
        if (viewports.empty()) return;
        auto vp = world.getComponent<ViewportComponent>(viewports[0]);
        auto drawableSize = ctx.renderer->getDrawableSize();
        auto vpData = vp.getData({drawableSize[0], drawableSize[1]});
        auto projectionMatrix = glm::perspective(glm::radians(camera->fov), vpData.first, camera->nearPlane, camera->farPlane);
        auto mr = App::mouseRay(activeCameraComponent.camera->getViewMatrix(), projectionMatrix, ctx.window, ctx.renderer);
        mouseRay.origin = mr[0];
        mouseRay.direction = mr[1];
    }

};

struct RenderRegistrationSystem : ISystem
{
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

struct SetLightsSystem : ISystem
{
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

struct MouseIntersectionSystem : public ISystem
{
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

struct UpdateLightsWithTransformSystem : ISystem
{
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

struct UpdateMaterialTintSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<TintMaterialComponent, RenderableComponent>();
        for (auto& entity : entities)
        {
            auto& tintComp = world.getComponent<TintMaterialComponent>(entity);
            auto& renderableComp = world.getComponent<RenderableComponent>(entity);
            // get bytes from material
            auto bytes = std::vector<std::byte>(sizeof(Tint));
            memcpy(bytes.data(), &tintComp.material, sizeof(Tint));
            // set bytes to the Corresponding Type
            // setMaterial search for a material of given type for the given renderable, and if found
            // data is overridden
            ctx.renderer->setMaterial(bytes, TintType, renderableComp.id);
        }
    }
};

struct MeshLoadingSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<MeshRequestComponent>();
        for (auto e : entities)
        {
            std::cout << "Loading mesh: " << world.getComponent<MeshRequestComponent>(e).path << std::endl;
            auto path = world.getComponent<MeshRequestComponent>(e).path;
            if (auto mesh = ctx.assetManager->getMesh(path))
            {
                world.addComponent<MeshComponent>(e, {mesh, path});
                world.removeComponent<MeshRequestComponent>(e);
            }
        }
    }
};
struct RendererUpdateSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto cameras = world.query<CameraComponent>();
        if (cameras.empty()) return;
        auto camera = world.getComponent<CameraComponent>(cameras[0]).camera;
        auto viewports = world.query<ViewportComponent>();
        if (viewports.empty()) return;
        auto vp = world.getComponent<ViewportComponent>(viewports[0]);
        auto drawableSize = ctx.renderer->getDrawableSize();
        auto [aspectRatio, normalizedViewport] = vp.getData({drawableSize[0], drawableSize[1]});
        auto projectionMatrix = glm::perspective(glm::radians(camera->fov), aspectRatio, camera->nearPlane, camera->farPlane);
        ctx.renderer->update(camera->getViewMatrix(), projectionMatrix, normalizedViewport);
    }
};

struct RectMaterialComponentSystem final : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        for (const auto entities = world.query<RectMaterialComponent, RenderableComponent>(); auto& entity : entities)
        {
            auto bytes = std::vector<std::byte>(sizeof(RectMaterial));
            auto& [material] = world.getComponent<RectMaterialComponent>(entity);
            auto& [id] = world.getComponent<RenderableComponent>(entity);
            memcpy(bytes.data(), &material, sizeof(RectMaterial));
            ctx.renderer->setMaterial(bytes, RectType, id);
        }
    }
};

// find viewport size material, then look for a viewport, and if found get the size and apply to material,
// then check the render config associated to the viewport size material and set it to the pso through the renderer
struct ViewportSizeMaterialSystem final : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        for (const auto entities = world.query<ViewportSizeMaterialComponent, RenderConfigComponent>(); auto& entity : entities)
        {
            auto viewportEntity = world.query<ViewportComponent>();
            if (viewportEntity.empty()) return;
            auto viewport = world.getComponent<ViewportComponent>(viewportEntity[0]);
            auto viewportData = viewport.getData({ctx.renderer->getDrawableSize()[0], ctx.renderer->getDrawableSize()[1]}, false);
            auto bytes = std::vector<std::byte>(sizeof(ViewportSize));
            auto& [material] = world.getComponent<ViewportSizeMaterialComponent>(entity);
            material.width = viewportData.second[2];
            material.height = viewportData.second[3];
            auto& renderConfig = world.getComponent<RenderConfigComponent>(entity);
            memcpy(bytes.data(), &material, sizeof(ViewportSize));
            ctx.renderer->setMaterial(renderConfig.psoName, bytes, ViewportSizeType);
            break;
        }
    }
};

#endif //EVOLVING_PLANETS_SYSTEMS_HPP