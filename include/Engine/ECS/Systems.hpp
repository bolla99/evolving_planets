//
// Created by Giovanni Bollati on 03/02/26.
//

#ifndef EVOLVING_PLANETS_SYSTEMS_HPP
#define EVOLVING_PLANETS_SYSTEMS_HPP

#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include "World.hpp"
#include "Rendering/IRenderer.hpp"
#include "Components.hpp"
#include "App.hpp"
#include "timer.hpp"

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
    virtual std::string name() const = 0;
    virtual ~ISystem() = default;
};

// this system takes entities that has a transform and a renderable id, and update the renderable model matrix
// from their transform
struct TransformSystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<Transform>();
        for (auto& entity : entities)
        {
            auto& transform = world.getComponent<Transform>(entity);

            if (world.hasComponent<MaterialComponent>(entity))
            {
                auto& material = world.getComponent<MaterialComponent>(entity);
                ctx.renderer->setInstanceMaterial(material.id, getBytes(transform.previousModelMatrix), MaterialType::PREVIOUS_MODEL_MATRIX);
                ctx.renderer->setInstanceMaterial(material.id, getBytes(transform.modelMatrix()), MaterialType::MODEL_MATRIX);
                ctx.renderer->setInstanceMaterial(material.id, getBytes(glm::inverseTranspose(transform.modelMatrix())), MaterialType::NORMAL_MATRIX);
            }
            if (world.hasComponent<PointLightComponent>(entity))
            {
                auto& light = world.getComponent<PointLightComponent>(entity).light;
                light.position = glm::vec4(transform.position, 1.0f);
            }
            if (world.hasComponent<DirectionalLightComponent>(entity))
            {
                auto& light = world.getComponent<DirectionalLightComponent>(entity).light;
                light.direction = glm::normalize(transform.rotation * light.direction);
            }

            transform.previousModelMatrix = transform.modelMatrix();
        }
    }
    [[nodiscard]] std::string name() const override { return "TransformSystem"; }
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
    std::string name() const override { return "MouseRaySystem"; }
};

struct RenderRegistrationSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<MeshComponent, RenderConfigComponent, RenderRequestComponent>();
        for (auto& entity : entities)
        {
            {
                // skip if entity has pending texture request
                if (world.hasComponent<TexturesRequestComponent>(entity)) continue;
                auto textures = std::vector<std::shared_ptr<Texture>>();
                // if textures are present, take them and use for the renderable builder
                if (world.hasComponent<TexturesComponent>(entity))
                {
                    textures = world.getComponent<TexturesComponent>(entity).textures;
                }
                // create renderable from mesh
                auto& meshComp = world.getComponent<MeshComponent>(entity);
                auto renderableID = ctx.renderer->addRenderable(*meshComp.mesh, textures);
                world.addComponent<RenderableComponent>(entity, {renderableID});
                // create material
                auto renderConfig = world.getComponent<RenderConfigComponent>(entity);
                auto materialID = ctx.renderer->addDefaultInstanceMaterial(renderConfig.psoName);
                world.addComponent<MaterialComponent>(entity, {materialID});

                // remove renderrequestcomponent
                world.removeComponent<RenderRequestComponent>(entity);
            }
        }
    }
    std::string name() const override { return "RenderRegistrationSystem"; }
};

// THIS SYSTEM SEARCH FOR LIGHTS AND SET LIGHTS AND SHADOWDATA GLOBAL MATERIALS
struct LightAndShadowSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        // SET LIGHTS
        auto directionalLights = world.query<DirectionalLightComponent>();
        auto pointLights = world.query<PointLightComponent>();
        auto ambientLight = world.query<AmbientLightComponent>();

        auto lights = Lights();
        auto shadowData = ShadowData();

        auto directionalLightsSize = std::min(MAX_DIRECTIONAL_LIGHTS, static_cast<int>(directionalLights.size()));
        auto pointLightsSize = std::min(MAX_POINT_LIGHTS, static_cast<int>(pointLights.size()));
        for (int i = 0; i < directionalLightsSize; i++)
        {
            auto lightComp = world.getComponent<DirectionalLightComponent>(directionalLights[i]);
            // loop through the passess for a single light
            for (int p = 0; p < lightComp.nPasses; p++)
            {
                lightComp.light.color *= lightComp.intensity;
                lights.directionalLights[i] = lightComp.light;
                lights.numDirectionalLights++;

                auto dir = glm::normalize(glm::vec3(lightComp.light.direction));

                // set first directional light as sun_direction
                if (i == 0)
                {
                    ctx.renderer->setGlobalMaterial(getBytes(glm::vec4(dir, 0.0f)), SUN_DIRECTION);
                    ctx.renderer->setGlobalMaterial(getBytes(lightComp.light.color * lightComp.intensity), SUN_COLOR);
                }
                auto settings = lightComp.settings[p];
                auto up = glm::vec3(0.0f, 1.0f, 0.0f);
                if (std::abs(glm::dot(glm::normalize(dir), up)) > 0.99f) up = glm::vec3(0.0f, 0.0f, 1.0f);
                auto lightViewMatrix = glm::lookAt(-dir * settings.distance + settings.center, settings.center, up);
                auto d = settings.orthoSize;
                auto orthoProjMatrix = glm::orthoRH_ZO(-1.0f * d, d, -1.0f * d, d, settings.nearPlane, settings.farPlane);
                shadowData.lightViewMatrix[i + p] = orthoProjMatrix * lightViewMatrix;
            }
        }

        auto pointShadowData = PointShadowData();
        for (int i = 0; i < pointLightsSize; i++)
        {
            auto lightComp = world.getComponent<PointLightComponent>(pointLights[i]);
            lightComp.light.color *= lightComp.intensity;
            lights.pointLights[i] = lightComp.light;
            lights.numPointLights++;

            auto fov = 90.0f;
            auto aspect = 1.0f;
            auto nearPlane = 0.01f;
            auto farPlane = 100.0f;
            auto lightProjMatrix = glm::perspectiveRH_ZO(glm::radians(fov), aspect, nearPlane, farPlane);
            //lightProjMatrix[1][1] *= -1;
            auto pos = glm::vec3(lightComp.light.position);
            auto viewMatrix1 = glm::lookAt(pos, pos + glm::vec3(1.0f, 0.0f, 0.0f), {0.0f, -1.0f, 0.0f});
            auto viewMatrix2 = glm::lookAt(pos, pos + glm::vec3(-1.0f, 0.0f, 0.0f), {0.0f, -1.0f, 0.0f});
            auto viewMatrix3 = glm::lookAt(pos, pos + glm::vec3(0.0f, 1.0f, 0.0f), {0.0f, 0.0f, 1.0f});
            auto viewMatrix4 = glm::lookAt(pos, pos + glm::vec3(0.0f, -1.0f, 0.0f), {0.0f, 0.0f, -1.0f});
            auto viewMatrix5 = glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, 1.0f), {0.0f, -1.0f, 0.0f});
            auto viewMatrix6 = glm::lookAt(pos, pos + glm::vec3(0.0f, 0.0f, -1.0f), {0.0f, -1.0f, 0.0f});

            pointShadowData.matrices[i * 6 + 0] = lightProjMatrix * viewMatrix1;
            pointShadowData.matrices[i * 6 + 1] = lightProjMatrix * viewMatrix2;
            pointShadowData.matrices[i * 6 + 2] = lightProjMatrix * viewMatrix3;
            pointShadowData.matrices[i * 6 + 3] = lightProjMatrix * viewMatrix4;
            pointShadowData.matrices[i * 6 + 4] = lightProjMatrix * viewMatrix5;
            pointShadowData.matrices[i * 6 + 5] = lightProjMatrix * viewMatrix6;

            pointShadowData.positions[i] = lightComp.light.position;
            pointShadowData.farPlanes[i] = glm::vec4(farPlane, farPlane, farPlane, farPlane);
        }
        if (!ambientLight.empty())
        {
            lights.globalAmbientLightColor = glm::vec4(world.getComponent<AmbientLightComponent>(ambientLight[0]).light, 1.0f);
        }

        ctx.renderer->setGlobalMaterial(getBytes(lights), LIGHTS);
        ctx.renderer->setGlobalMaterial(getBytes(shadowData), SHADOW_DATA);
        ctx.renderer->setGlobalMaterial(getBytes(pointShadowData), POINT_SHADOW_DATA);
    }
    std::string name() const override { return "SetLightsSystem"; }
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
    std::string name() const override { return "MouseIntersectionSystem"; }
};

/*
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
    std::string name() const override { return "UpdateLightsWithTransformSystem"; }
};
*/

/*
struct UpdateMaterialTintSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<TintMaterialComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto& tintComp = world.getComponent<TintMaterialComponent>(entity);
            auto mID = world.getComponent<MaterialComponent>(entity).id;
            // get bytes from material
            auto bytes = getBytes(tintComp.material);
            // check if the instance material has this particular material
            ctx.renderer->setInstanceMaterial(mID, bytes, MaterialType::TINT);
        }
    }
    std::string name() const override { return "UpdateMaterialTintSystem"; }
};
*/

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
    std::string name() const override { return "MeshLoadingSystem"; }
};

struct RendererUpdateSystem : ISystem
{
    glm::mat4 previousViewProjectionMatrix = glm::mat4(1.0f);

    void update(World& world, const Context& ctx, float dt) override
    {
        auto drawableSize = ctx.renderer->getDrawableSize();
        // JITTER
        // Puoi metterli in una variabile globale o statica
        static int frameIndex = 0;
        frameIndex = (frameIndex + 1) % 32; // Cicla tra 0 e 15
        // Esempio di sequenza di Halton precalcolata (in range -0.5 a +0.5 pixel)
        // Sequenza corretta e scalata tra -0.5 e +0.5
        static const float haltonX[32] = {
            0.00000f, -0.50000f,  0.25000f, -0.25000f,  0.37500f, -0.12500f,  0.12500f, -0.37500f,
            0.43750f, -0.06250f,  0.18750f, -0.31250f,  0.31250f, -0.18750f,  0.06250f, -0.43750f,
            0.46875f, -0.03125f,  0.21875f, -0.28125f,  0.34375f, -0.15625f,  0.09375f, -0.40625f,
            0.40625f, -0.09375f,  0.15625f, -0.34375f,  0.28125f, -0.21875f,  0.03125f, -0.46875f
        };

        static const float haltonY[32] = {
            -0.16667f,  0.16667f, -0.38889f,  0.05556f,  0.38889f, -0.27778f,  0.05556f,  0.27778f,
            -0.46296f, -0.12963f,  0.20370f, -0.35185f, -0.01852f,  0.31481f, -0.24074f,  0.09259f,
             0.42593f, -0.31481f,  0.01852f,  0.35185f, -0.20370f,  0.12963f,  0.46296f, -0.42593f,
            -0.09259f,  0.24074f, -0.31481f,  0.01852f,  0.35185f, -0.20370f,  0.12963f,  0.46296f
        };
        // jitter in pixel space
        float jitterX_pixel = haltonX[frameIndex];
        float jitterY_pixel = haltonY[frameIndex];
        // apply jitter
        auto TAAScaling = ctx.renderer->getTAAScaling();
        int internalWidth = floor(drawableSize[0] * TAAScaling);
        int internalHeight = floor(drawableSize[1] * TAAScaling);
        glm::vec2 jitterNDC = {
            jitterX_pixel * 2.0f / static_cast<float>(internalWidth),
            -jitterY_pixel * 2.0f / static_cast<float>(internalHeight)
        };
        ctx.renderer->setJitter(glm::vec2(jitterX_pixel, jitterY_pixel));

        Timer t1;
        auto cameras = world.query<CameraComponent>();
        if (cameras.empty()) return;
        auto camera = world.getComponent<CameraComponent>(cameras[0]).camera;
        auto viewports = world.query<ViewportComponent>();
        if (viewports.empty()) return;
        auto vp = world.getComponent<ViewportComponent>(viewports[0]);
        auto [aspectRatio, normalizedViewport] = vp.getData({drawableSize[0], drawableSize[1]});
        auto projectionMatrix = glm::perspectiveRH_ZO(glm::radians(camera->fov), aspectRatio, camera->nearPlane, camera->farPlane);

        // build render queue
        auto renderables = world.query<RenderableComponent, MaterialComponent, RenderConfigComponent>();
        auto queue = RenderQueue();
        for (auto& entity : renderables)
        {
            auto renderConfig = world.getComponent<RenderConfigComponent>(entity);
            if (!renderConfig.visible) continue;
            auto& renderable = world.getComponent<RenderableComponent>(entity);
            auto& material = world.getComponent<MaterialComponent>(entity);
            queue.add(RenderItem(renderable.id, material.id, renderConfig.castShadow, renderConfig.wireframe, renderConfig.gridSize, renderConfig.threadgroupSize), renderConfig.psoName, renderConfig.layer);
        }
        // update view matrix e projection matrix
        ctx.renderer->setGlobalMaterial(getBytes(camera->getViewMatrix()), MaterialType::VIEW_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::vec4{camera->getPosition(), 1.0f}), MaterialType::CAMERA_POSITION);
        ctx.renderer->setGlobalMaterial(getBytes(projectionMatrix), MaterialType::PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::inverse(camera->getViewMatrix())), MaterialType::INVERSE_VIEW_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::inverse(projectionMatrix)), MaterialType::INVERSE_PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(projectionMatrix * camera->getViewMatrix()), VIEW_PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::inverse(projectionMatrix * camera->getViewMatrix())), INVERSE_VIEW_PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(previousViewProjectionMatrix), PREVIOUS_VIEW_PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::vec2(camera->nearPlane, camera->farPlane)), CAMERA_PLANES);

        if (ctx.renderer->useTAAScaling)
        {
            ctx.renderer->setGlobalMaterial(getBytes(jitterNDC), JITTER);
        }
        else
        {
            ctx.renderer->setGlobalMaterial(getBytes(glm::vec2(0.0f)), JITTER);
        }

        //update previous projection matrix
        previousViewProjectionMatrix = projectionMatrix * camera->getViewMatrix();

        if (ctx.renderer->useTAAScaling)
        {
            projectionMatrix[2][0] += jitterNDC.x;
            projectionMatrix[2][1] += jitterNDC.y;
        }
        ctx.renderer->setGlobalMaterial(getBytes(projectionMatrix * camera->getViewMatrix()), JITTERED_VIEW_PROJECTION_MATRIX);
        ctx.renderer->setGlobalMaterial(getBytes(glm::inverse(projectionMatrix)), JITTERED_INVERSE_PROJECTION_MATRIX);

        ctx.renderer->update(queue, normalizedViewport);
    }
    std::string name() const override { return "RendererUpdateSystem"; }
};

struct RectMaterialComponentSystem final : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        for (const auto entities = world.query<RectMaterialComponent, MaterialComponent>(); auto& entity : entities)
        {
            auto& [material] = world.getComponent<RectMaterialComponent>(entity);
            auto& [id] = world.getComponent<MaterialComponent>(entity);
            auto bytes = getBytes(material);
            ctx.renderer->setInstanceMaterial(id, bytes, RECT);
        }
    }
    std::string name() const override { return "RectMaterialComponentSystem"; }
};

// find viewport size material, then look for a viewport, and if found get the size and apply to material,
// then check the render config associated to the viewport size material and set it to the pso through the renderer
struct ViewportSizeMaterialSystem final : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        for (const auto entities = world.query<ViewportSizeMaterialComponent, RenderConfigComponent>(); auto& entity : entities)
        {
            // get viewport
            auto viewportEntity = world.query<ViewportComponent>();
            if (viewportEntity.empty()) return;
            auto viewport = world.getComponent<ViewportComponent>(viewportEntity[0]);
            auto viewportData = viewport.getData({ctx.renderer->getDrawableSize()[0], ctx.renderer->getDrawableSize()[1]}, false);
            // get material
            auto& [material] = world.getComponent<ViewportSizeMaterialComponent>(entity);
            // update material
            material.width = viewportData.second[2];
            material.height = viewportData.second[3];
            // get bytes
            auto bytes = getBytes(material);

            auto renderConfig = world.getComponent<RenderConfigComponent>(entity);
            ctx.renderer->setGlobalMaterial(renderConfig.psoName, bytes, VIEWPORT_SIZE);
            ctx.renderer->setGlobalMaterial("UI", bytes, VIEWPORT_SIZE);
            ctx.renderer->setGlobalMaterial("TextureUI", bytes, VIEWPORT_SIZE);
            ctx.renderer->setGlobalMaterial("Depth", bytes, VIEWPORT_SIZE);

            break;
        }
    }
    std::string name() const override { return "ViewportSizeMaterialSystem"; }
};

struct TextureLoadingSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<TexturesRequestComponent>();
        for (auto e : entities)
        {
            auto txtPtrs = std::vector<std::shared_ptr<Texture>>();
            auto comp = world.getComponent<TexturesRequestComponent>(e);
            for (int i = 0; i < comp.paths.size(); i++)
            {
                auto txt = Texture::fromFile(comp.paths[i], comp.types[i]);
                if (txt) txtPtrs.push_back(txt);
            }
            world.addComponent<TexturesComponent>(e, TexturesComponent(txtPtrs));
            world.removeComponent<TexturesRequestComponent>(e);
        }
    }
    std::string name() const override { return "TextureLoadingSystem"; }
};

struct BoundingSphereRequestSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<BoundingSphereRequestComponent, MeshComponent>();
        for (auto e : entities)
        {
            auto& comp = world.getComponent<BoundingSphereRequestComponent>(e);
            auto& mesh = world.getComponent<MeshComponent>(e).mesh;
            world.addComponent<BoundingSphereComponent>(e, BoundingSphereComponent(glm::vec4(mesh->boundingSphereCenter(), mesh->boundingSphereRadius())));
            world.removeComponent<BoundingSphereRequestComponent>(e);
        }
    }
    std::string name() const override { return "BoundingSphereRequestSystem"; }
};

struct UpdateWardMaterialSystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<WardMaterialComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto& wardComp = world.getComponent<WardMaterialComponent>(entity);
            auto mID = world.getComponent<MaterialComponent>(entity).id;
            // get bytes from material
            auto roughness = getBytes(wardComp.roughness);
            auto metallic = getBytes(wardComp.metallic);
            auto alpha = getBytes(wardComp.alpha);
            // check if the instance material has this particular material
            ctx.renderer->setInstanceMaterial(mID, roughness, MaterialType::ROUGHNESS);
            ctx.renderer->setInstanceMaterial(mID, metallic, MaterialType::METALLIC);
            ctx.renderer->setInstanceMaterial(mID, alpha, WARD_ALPHA);
        }
    }
    std::string name() const override { return "Update Ward Material System"; }
};

struct UpdateCameraTransformSystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<CameraComponent, Transform>();
        for (auto& entity : entities)
        {
            auto& cameraComp = world.getComponent<CameraComponent>(entity);
            auto& transform = world.getComponent<Transform>(entity);
            transform.position = cameraComp.camera->getPosition();
        }
    }
    std::string name() const override { return "Update Ward Material System"; }
};

// look for entities that have a mesh and requiest for bvh component
struct UpdateBVH : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<BVHRequestComponent>();
        for (auto& entity : entities)
        {
            auto bvhComp = world.getComponent<BVHRequestComponent>(entity);
            auto mesh = bvhComp.mesh;
            if (!mesh) continue; // bvhrequestcomponent has not a valid mesh 
            // build bvh
            auto bvh = BVH(mesh->getVertices(), mesh->getFacesData(), bvhComp.primitivesPerLeaf);

            // add bvh component
            world.addComponent<BVHComponent>(entity, BVHComponent(bvh, mesh->getVertices(), mesh->getFacesData()));

            // remove request component
            world.removeComponent<BVHRequestComponent>(entity);
        }
    }
    std::string name() const override { return "Update Ward Material System"; }
};

struct UpdateBVHMaterial : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<BVHComponent, BVHMaterialComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto& bvhMatComp = world.getComponent<BVHMaterialComponent>(entity);
            if (bvhMatComp.dirty)
            {
                std::cout << "Updating bvh materials" << std::endl;
                auto bvhComp = world.getComponent<BVHComponent>(entity);
                auto materialComp = world.getComponent<MaterialComponent>(entity);
                auto mID = materialComp.id;
                auto bvh = bvhComp.bvh;
                auto vertices = bvhComp.vertices;
                auto indices = bvhComp.indices;
                ctx.renderer->setInstanceMaterial(mID, getBytes(bvh.getData()), BVH_NODES);
                ctx.renderer->setInstanceMaterial(mID, getBytes(bvh.getTriangles()), BVH_PRIMITIVES);

                // set them also as global material
                ctx.renderer->setGlobalMaterial(getBytes(bvh.getData()), BVH_NODES);
                ctx.renderer->setGlobalMaterial(getBytes(bvh.getTriangles()), BVH_PRIMITIVES);
                ctx.renderer->setGlobalMaterial(getBytes(BVHInfo{static_cast<int>(bvh.getData().size()), {}, static_cast<int>(bvh.getTriangles().size())}), BVH_INFO);

                bvhMatComp.dirty = false;
            }
        }
    }
    std::string name() const override { return "Update BVH Material System"; }
};

struct UpdateRenderingSettings : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<RenderingSettingsComponent>();
        for (auto& entity : entities)
        {
            auto& settings = world.getComponent<RenderingSettingsComponent>(entity);
            ctx.renderer->setTAAScaling(settings.TAAScaling);
            ctx.renderer->useTAAScaling = settings.useTAAScaling;
            ctx.renderer->setGlobalMaterial(getBytes(settings.TAAScaling), TAAScaling);
        }
    }
    std::string name() const override { return "Update BVH Material System"; }
};

struct UpdateBillboardMaterialsSystem : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<BillboardMaterialComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto& billboardComp = world.getComponent<BillboardMaterialComponent>(entity);
            auto mID = world.getComponent<MaterialComponent>(entity).id;
            // get bytes from material
            auto size = getBytes(billboardComp.material);
            // check if the instance material has this particular material
            ctx.renderer->setInstanceMaterial(mID, size, MaterialType::BILLBOARD_DATA);
        }
    }
    std::string name() const override { return "Update Billboard Material System"; }
};

struct MoveBillboards : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<BillboardMaterialComponent, Transform>();
        for (auto& entity : entities)
        {
            auto& billboardComp = world.getComponent<BillboardMaterialComponent>(entity);
            auto transform = world.getComponent<Transform>(entity);
            billboardComp.material.position = glm::vec4(transform.position, 1.0f);
        }
    }
    std::string name() const override { return "Move Bilboards System"; }
};

struct MoveChildren : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<Transform, ChildComponent>();
        for (auto& entity: entities)
        {
            auto childComponent = world.getComponent<ChildComponent>(entity);
            auto parentID = childComponent.parent;
            auto localTransform = childComponent.localTransform;
            auto& childGlobalTransform = world.getComponent<Transform>(entity);
            if (world.hasEntity(parentID) and world.hasComponent<Transform>(childComponent.parent))
            {
                // parent exists and has a component
                auto parentTransform = world.getComponent<Transform>(childComponent.parent);
                childGlobalTransform.position = parentTransform.position + (parentTransform.rotation * localTransform.position);
                childGlobalTransform.rotation = parentTransform.rotation * localTransform.rotation;
            }
        }
    }
    std::string name() const override { return "Move Children System"; }
};

struct UpdateUnlitMaterial : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<UnlitMaterialComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto mID = world.getComponent<MaterialComponent>(entity).id;
            auto unlitMaterial = world.getComponent<UnlitMaterialComponent>(entity);
            ctx.renderer->setInstanceMaterial(mID, getBytes(unlitMaterial.color), MaterialType::UNLIT_COLOR);
        }
    }
    std::string name() const override { return "Update Unlit Material System"; }
};

struct UpdateParticleMaterial : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<ParticleComponent, MaterialComponent>();
        for (auto& entity : entities)
        {
            auto mID = world.getComponent<MaterialComponent>(entity).id;
            auto material = world.getComponent<ParticleComponent>(entity);
            ctx.renderer->setInstanceMaterial(mID, getBytes(material.softness), MaterialType::PARTICLE_SOFTNESS);
        }
    }
    std::string name() const override { return "Update Particle Material System"; }
};

struct UpdateAtmosphereSettings : ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<AtmosphereSettingsComponent>();
        for (auto& entity : entities)
        {
            ctx.renderer->setGlobalMaterial(getBytes(world.getComponent<AtmosphereSettingsComponent>(entity).settings), ATMOSPHERE_SETTINGS);
        }
    }
    std::string name() const override { return "Update Atmosphere Settings System"; }
};


#endif //EVOLVING_PLANETS_SYSTEMS_HPP
