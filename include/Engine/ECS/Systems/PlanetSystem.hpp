#ifndef PLANET_SYSTEM_HPP
#define PLANET_SYSTEM_HPP

#include "Planet.hpp"
#include "Mesh.hpp"
#include "Engine/ECS/Systems.hpp"
#include "Engine/ECS/World.hpp"
#include "Engine/ECS/Components.hpp"
#include "GravityAdapter.hpp"

// get entities with planetconfigcomponent;
// if they are dirty, then update or create its planetdatacomponent
struct PlanetGeneratorSystem : public ISystem {

    void update(World& world, const Context& ctx, float dt) override {
        auto entities = world.query<PlanetConfigComponent>();
        for (auto entity : entities) {
            auto& config = world.getComponent<PlanetConfigComponent>(entity);

            if (config.isDirty) {
                // add planet data component if it does not exists
                if (!world.hasComponent<PlanetDataComponent>(entity))
                    world.addComponent<PlanetDataComponent>(entity, PlanetDataComponent());

                // update planet data component
                auto& data = world.getComponent<PlanetDataComponent>(entity);
                data.planet = Planet::sphere(config.nParallels, config.nMeridians, config.baseRadius);
                for (int i = 0; i < config.nMutations; ++i) {
                    data.planet->mutate(config.baseRadius / 5.0f, config.baseRadius / 2.0f, 0.01f, config.baseRadius);
                }

                // generate textures
                auto planetTexture = data.planet->toTexture(config.textureSize * 2, config.textureSize);
                auto normalTexture = data.planet->toNormalTexture(config.textureSize * 2, config.textureSize);

                // add texture component if it does not exist
                if (!world.hasComponent<TexturesComponent>(entity))
                    world.addComponent<TexturesComponent>(entity, TexturesComponent({}));

                // set textures
                auto& texturesComponent = world.getComponent<TexturesComponent>(entity);
                texturesComponent.textures.clear();
                texturesComponent.textures.push_back(planetTexture);
                texturesComponent.textures.push_back(normalTexture);

                // set global textures
                auto id = ctx.renderer->addGlobalTexture(texturesComponent.textures[0]);
                ctx.renderer->setGlobalTexture("Skybox", BSplineTexture, id);


                if (!world.hasComponent<PlanetInfoComponent>(entity))
                {
                    std::cout << "Planet Info Component missing" << std::endl;
                    throw std::runtime_error("Planet Info Component missing");
                }

                auto& planetInfoComponent = world.getComponent<PlanetInfoComponent>(entity);
                planetInfoComponent.info.planetRadius = config.baseRadius;

                auto mesh = Geometry::Mesh::fromIcoPlanetRockyfied(
                    *data.planet, 7, Geometry::Mesh::noVertexColor(),
                    false, 16,
                    planetInfoComponent.info.fractalIntensity, planetInfoComponent.info.fractalScale);

                if (world.hasComponent<MeshComponent>(entity))
                {
                    world.getComponent<MeshComponent>(entity).mesh = mesh;
                }
                else
                {
                    world.addComponent<MeshComponent>(entity, MeshComponent(mesh));
                }

                // add bvh request component
                if (!world.hasComponent<BVHComponent>(entity) and (!world.hasComponent<BVHRequestComponent>(entity)))
                {
                    std::cout << "BVH Request Component missing: creatin a new one" << std::endl;
                    world.addComponent<BVHRequestComponent>(entity, BVHRequestComponent(mesh, 16));
                    world.addComponent<BVHMaterialComponent>(entity, BVHMaterialComponent());
                }


                auto bsRadius = glm::length(mesh->boundingSphereCenter()) + mesh->boundingSphereRadius();
                // update sun pass frustum
                auto lightCompEntity = world.query<DirectionalLightComponent>()[0];
                auto& lightComp = world.getComponent<DirectionalLightComponent>(lightCompEntity);

                bool bake = false;
                auto atmosphereSettings = world.query<AtmosphereSettingsComponent>();
                if (not atmosphereSettings.empty())
                {
                    bake = world.getComponent<AtmosphereSettingsComponent>(atmosphereSettings[0]).bakeDensityAndLightTransmittance;
                }
                if (bake)
                {
                    int textureSize = 64;
                    // add PotentialSamplingInfoComponent if not already present
                    if (!world.hasComponent<PotentialSamplingInfoComponent>(entity))
                    {
                        std::cout << "Adding potential sampling info component" << std::endl;
                        world.addComponent<PotentialSamplingInfoComponent>(entity, PotentialSamplingInfoComponent());
                    }
                    auto& comp = world.getComponent<PotentialSamplingInfoComponent>(entity);
                    auto min = glm::vec3(-600.0f, -600.0f, -600.0f);
                    float edge = 1200.0f;

                    auto densityData = GravityAdapter::Util::getDensity3DTexture(mesh, min, edge, 64, -15.0f);

                    // USE NON-ZERO DENSITY RADIOUS AS THE NEW SAMPLING SPACE FOR LIGHT TRASMITTANCE
                    auto nonZeroDensityRadius = densityData.second;

                    // pick the max radius: the textures must be big enough to contain the entire planet, but
                    // not bigger than the non zero density radius, because it would be wasted sampling
                    auto maxRadius = max(nonZeroDensityRadius + 50.0f, bsRadius);

                    min = glm::vec3(-maxRadius, -maxRadius, -maxRadius);
                    edge = maxRadius * 2.0f;

                    auto lightTransmittanceData = GravityAdapter::Util::getLightTransmittanceTexture(mesh, min, edge, textureSize, -15.0f, densityData.second + 50.0f, lightComp.light.direction);
                    lightTransmittanceData = lightTransmittanceData->upscale3D(256, 256, 256);
                    lightTransmittanceData->applyGaussianBlur3D(1.0f);
                    comp.material = {glm::vec4(min, 1.0f), edge, {}, densityData.second + 50.0f, {}};

                    // recompute density with new bouds
                    auto newDensityData = GravityAdapter::Util::getDensity3DTexture(mesh, min, edge, textureSize * 2.0f, -15.0f);
                    auto newDensityDataUpscaled = newDensityData.first->upscale3D(256, 256, 256);
                    newDensityDataUpscaled->applyGaussianBlur3D(1.0f);

                    ctx.renderer->setGlobalTexture(Core::AtmosphereDensity3D, ctx.renderer->addGlobalTexture(newDensityDataUpscaled));
                    ctx.renderer->setGlobalTexture(Core::LightTransmittance3D, ctx.renderer->addGlobalTexture(lightTransmittanceData));
                }

                // if it was dirty and is already rendering, delete renderable and recreate with new textures
                // material is unchanged
                if (world.hasComponent<RenderableComponent>(entity))
                {
                    auto& renderableComponent = world.getComponent<RenderableComponent>(entity);
                    ctx.renderer->removeRenderable(renderableComponent.id);
                    renderableComponent.id = ctx.renderer->addRenderable(*world.getComponent<MeshComponent>(entity).mesh, world.getComponent<TexturesComponent>(entity).textures);
                }

                // add rigid body component
                if (!world.hasComponent<RigidBodyComponent>(entity))
                {
                   auto& rb = world.addComponent<RigidBodyComponent>(entity, RigidBodyComponent(ColliderType::BVH));
                    rb.mass = 0.0f;
                    rb.active = true;
                }

                config.isDirty = false;
            }

            if (!world.hasComponent<RenderConfigComponent>(entity))
            {
                auto rcc = RenderConfigComponent("PlanetShader");
                rcc.gridSize = {5120, 1, 1};
                rcc.threadgroupSize = {64, 1, 1};
                rcc.castShadow = 0xFFFFFFFF;
                world.addComponent<RenderConfigComponent>(entity, rcc);
            }
            // add render request component if neither request or renderable are present
            if (!world.hasComponent<RenderableComponent>(entity) and !world.hasComponent<RenderRequestComponent>(entity))
            {
                world.addComponent<RenderRequestComponent>(entity, RenderRequestComponent());
            }
        }
    }

    std::string name() const override { return "PlanetGeneratorSystem"; }
};

// UPDATE MATERIAL COMPONENT
struct PlanetMaterialSystem : public ISystem {
    void update(World& world, const Context& ctx, float dt) override {
        auto entities = world.query<PlanetInfoComponent, MaterialComponent>();
        for (auto entity : entities) {
            auto& data = world.getComponent<PlanetInfoComponent>(entity);
            // update constant LOD
            if (data.info.useConstantLOD)
            {
                // check if texture exists
                if (world.hasComponent<TexturesComponent>(entity))
                {
                    auto& textures = world.getComponent<TexturesComponent>(entity).textures;
                    std::shared_ptr<Texture> bsplineTexture = nullptr;
                    for (int i = 0; i < textures.size(); ++i)
                    {
                        if (textures[i]->type() == TextureType::BSplineTexture)
                        {
                            bsplineTexture = textures[i];
                            break;
                        }
                    }
                    if (!bsplineTexture) return;

                    // get camera
                    auto camera = world.getActiveCamera();
                    if (camera == 0) return;
                    auto cameraComponent = world.getComponent<CameraComponent>(camera);
                    auto cam = cameraComponent.camera;
                    auto cameraPos = cam->getPosition();
                    // get viewport
                    auto viewports = world.query<ViewportComponent>();
                    if (viewports.empty()) return;
                    auto vp = world.getComponent<ViewportComponent>(viewports[0]);
                    auto drawableSize = ctx.renderer->getDrawableSize();
                    auto [aspectRatio, normalizedViewport] = vp.getData({drawableSize[0], drawableSize[1]});
                    auto projectionMatrix = glm::perspectiveRH_ZO(glm::radians(cam->fov), aspectRatio, cam->nearPlane, cam->farPlane);
                    data.info.constantLOD = Planet::calculateLOD(bsplineTexture, cameraPos, projectionMatrix, data.info.planetRadius);
                }
            }
            auto& matComp = world.getComponent<MaterialComponent>(entity);
            // get bytes from material
            auto bytes = getBytes(data.info);

            ctx.renderer->setInstanceMaterial(matComp.id, bytes, MaterialType::COMPACT_PLANET_INFO);
        }
    }

    [[nodiscard]] std::string name() const override { return "PlanetMaterialSystem"; }
};

struct UpdateOctreeMaterial : public ISystem {
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<OctreeComponent>();
        for (auto& entity : entities)
        {
            auto octreeComponent = world.getComponent<OctreeComponent>(entity);
            PotentialOctreeInfo octreeInfo;
            octreeInfo.size = octreeComponent.octree.size();
            octreeInfo.minBounds = glm::vec4(octreeComponent.min, 0.0f);
            octreeInfo.edge = octreeComponent.edge;
            octreeInfo.multiplier = octreeComponent.multiplier;

            ctx.renderer->setGlobalMaterial(getBytes(octreeComponent.octree), MaterialType::POTENTIAL_OCTREE);
            ctx.renderer->setGlobalMaterial(getBytes(octreeInfo), MaterialType::POTENTIAL_OCTREE_INFO);
        }
    }

    [[nodiscard]] std::string name() const override { return "UpdateOctreeMaterial"; }
};

struct UpdatePotentialSamplingInfoMaterial : public ISystem {
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<PotentialSamplingInfoComponent>();
        for (auto& entity : entities)
        {
            auto samplingInfoComponent = world.getComponent<PotentialSamplingInfoComponent>(entity);
            ctx.renderer->setGlobalMaterial(getBytes(samplingInfoComponent.material), MaterialType::POTENTIAL_SAMPLING_INFO);
        }
    }
    [[nodiscard]] std::string name() const override { return "UpdatePotentialSamplingInfoMaterial"; }
};

struct UpdatePotentialFieldDebugValue : public ISystem {
    void update(World& world, const Context& ctx, float dt) override
    {
        auto entities = world.query<PotentialSamplingInfoComponent, PotentialFieldComponent>();
        for (auto& entity : entities)
        {
            auto samplingInfoComponent = world.getComponent<PotentialSamplingInfoComponent>(entity);
            auto& fieldComponent = world.getComponent<PotentialFieldComponent>(entity);

            // get player
            auto players = world.query<PlayerComponent>();
            if (players.empty()) break;
            auto transform = world.getComponent<Transform>(players[0]);
            auto normalizedPosition = (glm::vec3(transform.position) - glm::vec3(samplingInfoComponent.material.min)) / samplingInfoComponent.material.edge;
            fieldComponent.debugValue = fieldComponent.potentialTexture->sample(normalizedPosition.x, normalizedPosition.y, normalizedPosition.z).x;
        }
    }
    [[nodiscard]] std::string name() const override { return "UpdatePotentialFieldDebugValue"; }
};




#endif //PLANET_SYSTEM_HPP
