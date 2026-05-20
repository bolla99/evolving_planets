#ifndef PLANET_SYSTEM_HPP
#define PLANET_SYSTEM_HPP

#include "Planet.hpp"
#include "Engine/ECS/Systems.hpp"
#include "Engine/ECS/World.hpp"
#include "Engine/ECS/Components.hpp"

// get entities with planetconfigcomponent;
// if they are dirty, then update or create its planetdatacomponent
struct PlanetGeneratorSystem : public ISystem {
    void update(World& world, const Context& ctx, float dt) override {
        auto entities = world.query<PlanetConfigComponent>();
        for (auto entity : entities) {
            auto& config = world.getComponent<PlanetConfigComponent>(entity);

            if (config.isDirty) {
                if (world.hasComponent<PlanetDataComponent>(entity))
                {
                    auto& data = world.getComponent<PlanetDataComponent>(entity);
                    data.planet = Planet::sphere(config.nParallels, config.nMeridians, config.baseRadius);
                    for (int i = 0; i < config.nMutations; ++i) {
                        data.planet->mutate(config.baseRadius / 5.0f, config.baseRadius / 2.0f);
                    }
                    auto& texturesComponent = world.getComponent<TexturesComponent>(entity);
                    texturesComponent.textures.clear();
                    texturesComponent.textures.push_back(data.planet->toTexture(config.textureSize, config.textureSize));
                    texturesComponent.textures.push_back(data.planet->toNormalTexture(config.textureSize, config.textureSize));

                    config.isDirty = false;
                }
                else
                {
                    auto& data = world.addComponent<PlanetDataComponent>(entity, PlanetDataComponent());
                    data.planet = Planet::sphere(config.nParallels, config.nMeridians, config.baseRadius);
                    for (int i = 0; i < config.nMutations; ++i) {
                        data.planet->mutate(config.baseRadius / 5.0f, config.baseRadius / 2.0f);
                    }
                    auto t1 = data.planet->toTexture(config.textureSize, config.textureSize);
                    auto t2 = data.planet->toNormalTexture(config.textureSize, config.textureSize);
                    auto& textures = world.addComponent<TexturesComponent>(entity, TexturesComponent({t1, t2}));

                    config.isDirty = false;
                }

                if (world.hasComponent<PlanetInfoComponent>(entity))
                {
                    auto& info = world.getComponent<PlanetInfoComponent>(entity);
                    info.info.planetRadius = config.baseRadius;
                }
                // if it was dirty and is already rendering, delete renderable and recreate with new textures
                // material is unchanged
                if (world.hasComponent<RenderableComponent>(entity))
                {
                    auto& renderableComponent = world.getComponent<RenderableComponent>(entity);
                    ctx.renderer->removeRenderable(renderableComponent.id);
                    renderableComponent.id = ctx.renderer->addRenderable(*world.getComponent<MeshComponent>(entity).mesh, world.getComponent<TexturesComponent>(entity).textures);
                }

                if (!world.hasComponent<RenderConfigComponent>(entity))
                {
                    auto rcc = RenderConfigComponent("PlanetShader");
                    rcc.gridSize = {5120, 1, 1};
                    rcc.threadgroupSize = {64, 1, 1};
                    world.addComponent<RenderConfigComponent>(entity, rcc);
                }
                // add mesh component if not present
                if (!world.hasComponent<MeshComponent>(entity))
                {
                    world.addComponent<MeshComponent>(entity, MeshComponent(Geometry::Mesh::dummy()));
                }
                // add render request component if neither request or renderable are present
                if (!world.hasComponent<RenderableComponent>(entity) and !world.hasComponent<RenderRequestComponent>(entity))
                {
                    world.addComponent<RenderRequestComponent>(entity, RenderRequestComponent());
                }
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
                    auto cameras = world.query<CameraComponent>();
                    if (cameras.empty()) return;
                    auto camera = cameras[0];
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



#endif //PLANET_SYSTEM_HPP
