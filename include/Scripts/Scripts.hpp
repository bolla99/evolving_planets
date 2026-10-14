//
// Created by Giovanni Bollati on 01/07/26.
//

#ifndef EVOLVING_PLANETS_SCRIPTS_HPP
#define EVOLVING_PLANETS_SCRIPTS_HPP

#include <SDL_mouse.h>
#include <Engine/ECS/Context.hpp>
#include <Engine/ECS/World.hpp>

using ScriptFunc = std::function<void(World&, const Context&, uint64_t, float)>;

// MAIN CAMERA SCRIPT
// user input make the camera pan and tilt; position follows the player ship
inline ScriptFunc mainCameraScriptB = [](World& world, const Context& ctx, uint64_t entity, float dt) {
    float sensitivity = 0.002f;

    if (ctx.input and ctx.input->isJustPressed(SDL_SCANCODE_1))
    {
        world.setActiveCamera(entity);
    }

    // get camera component
    if (not world.hasComponent<CameraComponent>(entity)) return;
    auto& cameraComp = world.getComponent<CameraComponent>(entity);

    // move camera only if it is the active camera
    if (ctx.input && !ctx.input->isCursorActive and cameraComp.active)
    {
        auto camera = cameraComp.camera;
        auto fpsCamera = std::static_pointer_cast<FPSCamera>(camera);

        fpsCamera->localPan(-static_cast<float>(ctx.input->mouseXRel) * sensitivity);
        fpsCamera->tilt(-static_cast<float>(ctx.input->mouseYRel) * sensitivity);

        auto rollSpeed = 1.0f;
        if (ctx.input->isPressed(SDL_SCANCODE_U))
        {
            camera->roll(-dt * rollSpeed);
        }
        if (ctx.input->isPressed(SDL_SCANCODE_O))
        {
            camera->roll(dt * rollSpeed);
        }
    }
};

// debug camera is a free movement camera
// it pans and tilts with mouse, and moves with ijkl
inline ScriptFunc debugCameraScriptB = [](World& world, const Context& ctx, uint64_t entity, float dt)
{
    float sensitivity = 0.002f;
    auto camSpeed = 10.0f;

    if (not world.hasComponent<CameraComponent>(entity)) return;
    auto& cameraComp = world.getComponent<CameraComponent>(entity);
    auto camera = static_pointer_cast<FPSCamera>(cameraComp.camera);

    if (ctx.input and ctx.input->isJustPressed(SDL_SCANCODE_2))
    {
        world.setActiveCamera(entity);
        // find ship
        auto ship = world.query<PlayerComponent>();
        if (!ship.empty())
        {
            auto shipTransform = world.getComponent<Transform>(ship[0]);
            auto shipPosition = shipTransform.position;
            camera->setPosition(shipPosition);
        }
    }
    // move camera only if it is active
    if (ctx.input and !ctx.input->isCursorActive and cameraComp.active)
    {
        camera->localPan(-static_cast<float>(ctx.input->mouseXRel) * sensitivity);
        camera->tilt(-static_cast<float>(ctx.input->mouseYRel) * sensitivity);

        auto velocity = glm::vec2{0.0f, 0.0f};
        if (ctx.input->isPressed(SDL_SCANCODE_LEFT)) velocity.x -= 1.0f;
        if (ctx.input->isPressed(SDL_SCANCODE_RIGHT)) velocity.x += 1.0f;
        if (ctx.input->isPressed(SDL_SCANCODE_UP)) velocity.y += 1.0f;
        if (ctx.input->isPressed(SDL_SCANCODE_DOWN)) velocity.y -= 1.0f;
        camera->advance(velocity.y * camSpeed * dt);
        camera->strafe(velocity.x * camSpeed * dt);
        std::cout << "delta time inside debug camera script: " << dt << std::endl;
    }
};

inline ScriptFunc sunScriptB = [](World& world, const Context& ctx, uint64_t entity, float dt)
{
    if (not world.hasComponent<DirectionalLightComponent>(entity)) return;
    auto& lightComp = world.getComponent<DirectionalLightComponent>(entity);

    // get planet radius
    auto radius = 100.0f;
    auto planet = world.query<PlanetConfigComponent, MeshComponent>();
    if (not planet.empty())
    {
        auto meshComp = world.getComponent<MeshComponent>(planet[0]);
        radius = meshComp.mesh->boundingSphereRadius() + glm::length(meshComp.mesh->boundingSphereCenter());
    }

    // get player position
    glm::vec3 playerPosition = glm::vec3{0.0f, 0.0f, 0.0f};
    auto players = world.query<PlayerComponent, Transform>();
    if (not players.empty())
    {
        auto playerTransform = world.getComponent<Transform>(players[0]);
        playerPosition = playerTransform.position;
    }

    auto distance = glm::dot(glm::vec3(glm::normalize(lightComp.light.direction)), playerPosition);
    auto padding = 30.0f;
    // pass 0
    auto& pass0 = lightComp.settings[0];
    pass0.nearPlane = 0.1f;
    pass0.distance = padding + clamp(radius + distance, 0.0f, 1000000.0f);
    pass0.farPlane = abs(distance) + 2.0f * radius + padding;
    pass0.orthoSize = 50.0f;
    pass0.center = playerPosition;

    auto& pass1 = lightComp.settings[1];
    pass1.nearPlane = 0.1f;
    pass1.distance = padding + clamp(radius + distance, 0.0f, 1000000.0f);
    pass1.farPlane = abs(distance) + 2.0f * radius + padding;
    pass1.orthoSize = 150.0f;
    pass1.center = playerPosition;

    auto& pass2 = lightComp.settings[2];
    pass2.nearPlane = 0.1f;
    pass2.distance = padding + clamp(radius + distance, 0.0f, 1000000.0f);
    pass2.farPlane = abs(distance) + 2.0f * radius + padding;
    pass2.orthoSize = 300.0f;
    pass2.center = playerPosition;

    auto& pass3 = lightComp.settings[3];
    pass3.nearPlane = 0.1f;
    pass3.distance = radius + padding;
    pass3.farPlane = 2.0f * (radius + padding);
    pass3.orthoSize = 2.0f * (radius + padding);
    pass3.center = glm::vec3(0.0f);
};


#endif //EVOLVING_PLANETS_SCRIPTS_HPP
