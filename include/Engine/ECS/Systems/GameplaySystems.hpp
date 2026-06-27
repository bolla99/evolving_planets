//
// Created by Giovanni Bollati on 31/05/26.
//

#ifndef EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
#define EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
#include "Engine/ECS/Components.hpp"
#include "Engine/ECS/Systems.hpp"


struct UpdateCameraPositionSystem : public ISystem
{
    void update(World& world, const Context& ctx, float dt) override
    {
        auto cameras = world.query<CameraComponent>();
        if (cameras.empty()) return;
        auto camera =  cameras[0];
        auto players = world.query<PlayerComponent>();
        if (players.empty()) return;
        auto player = players[0];

        auto fpsCamera = static_pointer_cast<FPSCamera>(world.getComponent<CameraComponent>(camera).camera);
        auto worldPosition = world.getComponent<Transform>(player).position;
        auto centerOfMass = world.getComponent<MeshComponent>(player).mesh->centerOfMass();
        auto direction = glm::normalize(-30.0f * fpsCamera->front() + 8.0f * fpsCamera->up());
        auto distance = 12.0f;
        auto origin = worldPosition + centerOfMass;
        fpsCamera->setPosition(origin);

         // check if camera is colliding with the planet
        // find planet

        auto planets = world.query<PlanetConfigComponent, BVHComponent, MeshComponent>();
        if (!planets.empty())
        {
            // get viewport
            auto viewports = world.query<ViewportComponent>();
            if (!viewports.empty())
            {
                const auto& bvhComp = world.getComponent<BVHComponent>(planets[0]);
                const auto& bvh = bvhComp.bvh;

                // intersection occurs
                if (bvh.intersect(origin, direction).size() > 0)
                {
                    // correct intersection with camera frame
                    auto viewport = world.getComponent<ViewportComponent>(viewports[0]);
                    auto ds = ctx.renderer->getDrawableSize();
                    auto [aspectRatio, normalizedViewport] = viewport.getData(ds);
                    auto frame = fpsCamera->frameInWorldSpace(aspectRatio, 0.0f);

                    auto minT = std::numeric_limits<float>::max();
                    auto minPos = glm::vec3(minT);
                    bool found = false;
                    for (int i = 0; i < 4; i++) {
                        auto intersection = bvh.intersect(frame[i], direction);
                        if (!intersection.empty())
                        {
                            auto t = intersection[0];
                            if (t < minT)
                            {
                                minT = t;
                                minPos = frame[i] + t * direction;
                                found = true;
                            }
                        }
                    }
                    if (found)
                    {
                        auto v = minPos - origin;
                        distance = min(distance, glm::dot(v, direction));
                    }
                }
            }
        }

        //distance -= 5.0f;
        fpsCamera->setPosition(origin + direction * distance);
    }
    std::string name() const override { return "Update Camera Position System"; }
};


#endif //EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
