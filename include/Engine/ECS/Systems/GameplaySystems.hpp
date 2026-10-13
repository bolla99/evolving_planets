//
// Created by Giovanni Bollati on 31/05/26.
//

#ifndef EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
#define EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
#include "Engine/ECS/Components.hpp"
#include "Engine/ECS/Systems.hpp"
#include "glm/gtx/compatibility.hpp"


struct PlayerControllerSystem : public ISystem
{
    float thurst = 0.0f;
    float heat = 0.0f;

    void update(World& world, const Context& ctx, float dt) override
    {
        // get player
        auto players = world.query<PlayerComponent>();
        if (players.empty()) return;
        auto playerID = players[0];
        auto& playerComponent = world.getComponent<PlayerComponent>(playerID);

        // set cursor active
        if (ctx.input->isJustPressed(SDL_SCANCODE_SPACE))
        {
            ctx.input->isCursorActive = !ctx.input->isCursorActive;
            SDL_SetRelativeMouseMode(ctx.input->isCursorActive ? SDL_FALSE : SDL_TRUE);
        }

        auto cameras = world.query<CameraComponent, TagComponent>();
        auto mainCameraID = 0;
        for (auto camera : cameras)
        {
            if (world.getComponent<TagComponent>(camera).hasTag(MAIN_CAMERA))
            {
                mainCameraID = camera;
                break;
            }
        }
        if (mainCameraID == 0) return;

        auto cameraComp = world.getComponent<CameraComponent>(mainCameraID);
        auto camera = cameraComp.camera;

        // THE PLAYER IS ACTIVE AS LONG AS THE MAIN CAMERA IS ACTIVE
        playerComponent.active = cameraComp.active;

        // MOVE SHIP

        // PLAYER INPUT; if not active, dont move
        if (not playerComponent.active) return;

        if (world.hasComponent<RigidBodyComponent>(playerID))
        {
            auto& rigidb = world.getComponent<RigidBodyComponent>(playerID);
            if (auto body = rigidb.body)
            {
                auto forwardSpeed = 80000.0f;
                auto lateralSpeed = 10000.0f;
                auto shipTransform = world.getComponent<Transform>(playerID);

                body->setDamping(0.6f, 0.8f);

                if (ctx.input->isPressed(SDL_SCANCODE_I))
                {
                    auto dir = shipTransform.front();
                    rigidb.force += dir * forwardSpeed;

                    // THURSTING
                    thurst += dt * 2.0f;
                    thurst = clamp(thurst, 0.0f, 1.0f);

                    heat += dt * 1.0f;
                    heat = clamp(heat, 0.0f, 1.0f);
                }
                else
                {
                    thurst -= dt * 2.0f;
                    thurst = clamp(thurst, 0.0f, 1.0f);

                    heat -= dt * 0.1f;
                    heat = clamp(heat, 0.0f, 1.0f);
                }
                if (ctx.input->isPressed(SDL_SCANCODE_K))
                {
                    auto dir = -shipTransform.front();
                    rigidb.force += dir * lateralSpeed;
                }
                if (ctx.input->isPressed(SDL_SCANCODE_J))
                {
                    auto dir = -shipTransform.right();
                    rigidb.force += dir * lateralSpeed;
                }
                if (ctx.input->isPressed(SDL_SCANCODE_L))
                {
                    auto dir = shipTransform.right();
                    rigidb.force += dir * lateralSpeed;
                }
                if (ctx.input->isPressed(SDL_SCANCODE_H))
                {
                    auto dir = shipTransform.up();
                    rigidb.force += dir * lateralSpeed;
                }

                // rotate according to camera
                auto sFront = shipTransform.front();
                auto cFront = camera->front();
                btVector3 shipForward(sFront.x, sFront.y, sFront.z);
                btVector3 camForward(cFront.x, cFront.y, cFront.z);

                btVector3 errorForward = shipForward.cross(camForward);

                auto sUp = shipTransform.up();
                auto cUp = camera->up();
                btVector3 shipUp(sUp.x, sUp.y, sUp.z);
                btVector3 camUp(cUp.x, cUp.y, cUp.z);

                btVector3 errorUp = shipUp.cross(camUp);

                btVector3 totalError = errorForward + errorUp;

                float rotationSpeed = 0.9f;
                //body->setAngularVelocity(totalError * rotationSpeed);
                rigidb.torque += glm::vec3(totalError.x(), totalError.y(), totalError.z()) * rotationSpeed;
            }
        }

        // set thurst material
        ctx.renderer->setGlobalMaterial(getBytes(thurst), THURST);
        ctx.renderer->setGlobalMaterial(getBytes(heat), HEAT);
    }
    std::string name() const override { return "Player Controller System"; }
};

struct UpdateCameraPositionSystem : public ISystem
{
    float currentDistance = 12.0f;
    float targetDistance = 12.0f;
    float cameraSpeed = 5.0f;

    void update(World& world, const Context& ctx, float dt) override
    {
        auto debug = false;

        auto cameras = world.query<CameraComponent>();
        auto camera = 0;
        for (auto& c : cameras)
        {
            if (world.hasComponent<TagComponent>(c) && world.getComponent<TagComponent>(c).hasTag(MAIN_CAMERA)) {
                camera = c;
                break;
            }
        }
        if (camera == 0) return;
        auto players = world.query<PlayerComponent>();
        if (players.empty()) return;
        auto player = players[0];

        auto fpsCamera = static_pointer_cast<FPSCamera>(world.getComponent<CameraComponent>(camera).camera);

        auto worldPosition = world.getComponent<Transform>(player).position;
        auto mesh = world.getComponent<MeshComponent>(player).mesh;
        auto bsRadius = mesh->boundingSphereRadius();
        auto direction = glm::normalize(-30.0f * fpsCamera->front() + 8.0f * fpsCamera->up());
        auto origin = worldPosition + direction * bsRadius;

        if (debug) ctx.renderer->debugDrawSphere(worldPosition, bsRadius, glm::vec4(1.0f, 0.0f, 0.0f, 1.0f));
        auto distance = 12.0f;

        // set camera to ideal position
        auto originalPosition = fpsCamera->getPosition();
        fpsCamera->setPosition(origin + direction * distance);

        // draw debug axis
        if (debug)
        {
            ctx.renderer->debugDrawLine(origin, origin + direction * distance, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
            ctx.renderer->debugDrawSphere(origin + direction * distance, 0.2f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
            ctx.renderer->debugDrawSphere(origin, 0.2f, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        }

        targetDistance = 12.0f;

         // check if camera is colliding with the planet
        // find planet
        auto planets = world.query<PlanetConfigComponent, BVHComponent, MeshComponent>();
        if (!planets.empty())
        {
            // get viewport
            auto viewports = world.query<ViewportComponent>();
            if (!viewports.empty())
            {
                auto viewport = world.getComponent<ViewportComponent>(viewports[0]);
                auto ds = ctx.renderer->getDrawableSize();
                auto [aspectRatio, normalizedViewport] = viewport.getData(ds);

                auto bvh = world.getComponent<BVHComponent>(planets[0]).bvh;

                auto frame = fpsCamera->frameInWorldSpace(aspectRatio, 2.0f);
                auto offset = glm::dot(frame[0] - fpsCamera->getPosition(), -direction);

                auto maxT = std::numeric_limits<float>::min();
                bool found = false;
                for (int i = 0; i < 4; i++) {
                    if (debug) ctx.renderer->debugDrawLine(frame[i], frame[i] - direction * distance, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f));
                    auto intersection = bvh.intersect(frame[i], -direction);
                    if (!intersection.empty())
                    {
                        auto t = intersection[0];
                        if (t < distance and t > maxT)
                        {
                            auto impact = frame[i] - direction * t;
                            if (debug) ctx.renderer->debugDrawSphere(impact, 0.2f, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));
                            maxT = t;
                            found = true;
                        }
                    }
                }
                if (found)
                {
                    targetDistance = 12.0f - maxT - offset;
                    //auto camPosition = fpsCamera->getPosition();
                    //auto targetPosition = camPosition - direction * maxT;
                    //fpsCamera->setPosition(glm::mix(originalPosition, targetPosition, dt * cameraSpeed));
                }
            }
        }

        if (currentDistance != targetDistance)
        {
            currentDistance = glm::mix(currentDistance, targetDistance, dt * cameraSpeed);
        }
        /*else if (currentDistance < targetDistance)
        {
            currentDistance = targetDistance;
        }*/


        fpsCamera->setPosition(origin + direction * currentDistance);

        // check if the camera is currently inside the planet ; if it is, disable skirts
        auto ps = world.query<PlanetInfoComponent, BVHComponent>();
        if (not ps.empty())
        {
            auto camPosition = fpsCamera->getPosition();
            auto bvh = world.getComponent<BVHComponent>(ps[0]).bvh;
            auto intersection = bvh.intersect(camPosition, glm::normalize(camPosition));
            if (intersection.size() % 2 != 0)
            {
                world.getComponent<PlanetInfoComponent>(ps[0]).info.useSkirts = false;
            }
            else
            {
                world.getComponent<PlanetInfoComponent>(ps[0]).info.useSkirts = true;
            }
        }

        if (debug) ctx.renderer->debugDrawSphere(fpsCamera->getPosition(), 1.0f, glm::vec4(0.0f, 1.0f, 0.0f, 1.0f));
    }
    std::string name() const override { return "Update Camera Position System"; }
};


#endif //EVOLVING_PLANETS_GAMEPLAYSYSTEMS_HPP
