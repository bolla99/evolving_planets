//
// Created by Giovanni Bollati on 29/01/26.
//

#include <SDL.h>
#include <RTGPApp.hpp>
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "Engine/ECS/Storage.hpp"
#include "TrackballCamera.hpp"
#include "Engine/ECS/World.hpp"
#include "Apple/Util.hpp"
#include "../include/Engine/ECS/Components.hpp"
#include <Engine/ECS/Systems.hpp>
#include "timer.hpp"
#include "../include/Engine/ECS/SystemsManager.hpp"
#include "Engine/UI/ComponentsInspector.hpp"
#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include "Engine/ECS/Systems/GameplaySystems.hpp"
#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/PlanetSystem.hpp"

void RTGPApp::init()
{}

void RTGPApp::run()
{
    // INPUT: SET CONTROLLER
    SDL_GameController* controller = nullptr;
    for (int i = 0; i < SDL_NumJoysticks(); i++)
    {
        if (SDL_IsGameController(i))
        {
            controller = SDL_GameControllerOpen(i);
            if (controller)
            {
                std::cout << "Controller connected: " << SDL_GameControllerName(controller) << std::endl;
                break;
            }
        }
        std::cout << "Controller not connected" << std::endl;
    }

    bool debugCamera = false;

    // CREATE WORLD AND CONTEXT
    auto world = World();
    auto ctx = Context{_renderer.get(), _window, &_assetManager};
    auto inspector = ComponentsInspector::factory();

    // SYSTEMS MANAGER
    auto systemManager = SystemsManager();
    // LOADING AND REGISTRATION
    systemManager.addSystem<PlanetGeneratorSystem>();
    systemManager.addSystem<MeshLoadingSystem>();
    systemManager.addSystem<TextureLoadingSystem>();
    systemManager.addSystem<RenderRegistrationSystem>();
    systemManager.addSystem<BoundingSphereRequestSystem>();
    systemManager.addSystem<UpdateBVH>();
    // GAMEPLAY SYSTEMS
    systemManager.addSystem<MouseRaySystem>();
    systemManager.addSystem<MouseIntersectionSystem>();
    // UPDATE MATERIALS
    systemManager.addSystem<PlanetMaterialSystem>();
    systemManager.addSystem<UpdateWardMaterialSystem>();
    systemManager.addSystem<RectMaterialComponentSystem>();
    systemManager.addSystem<ViewportSizeMaterialSystem>();
    systemManager.addSystem<UpdateBVHMaterial>();
    systemManager.addSystem<UpdateUnlitMaterial>();
    systemManager.addSystem<UpdateParticleMaterial>();
    systemManager.addSystem<UpdateOctreeMaterial>();
    systemManager.addSystem<UpdatePotentialSamplingInfoMaterial>();
    systemManager.addSystem<UpdatePotentialFieldDebugValue>();
    systemManager.addSystem<UpdateAtmosphereSettings>();

    // UPDATE PHYSICS
    systemManager.addSystem<PhysicsSystem>();
    // MOVE CHILDREN
    systemManager.addSystem<MoveChildren>();
    // MOVE BILLBOARDS
    systemManager.addSystem<MoveBillboards>();
    systemManager.addSystem<UpdateBillboardMaterialsSystem>();

    // GAMEPLAY SYSTEMS
    systemManager.addSystem<UpdateCameraPositionSystem>();
    // APPLY TRANSFORM TO GRAPHICS
    systemManager.addSystem<TransformSystem>();
    systemManager.addSystem<UpdateCameraTransformSystem>();
    // UPDATE LIGHTS
    systemManager.addSystem<LightAndShadowSystem>();
    // GRAPHICS UPDATE
    systemManager.addSystem<UpdateRenderingSettings>();
    systemManager.addSystem<RendererUpdateSystem>();

    // ENTITIES

    // viewport
    auto viewport = world.createEntity("viewport");
    world.addComponent<ViewportComponent>(viewport, ViewportComponent());

    // planet
    auto planet = world.createEntity("planet");
    world.addComponent<PlanetConfigComponent>(planet, PlanetConfigComponent());
    world.addComponent<Transform>(planet, Transform());
    world.addComponent<PlanetInfoComponent>(planet, PlanetInfoComponent());

    // rendering settings
    auto renderingSettings = world.createEntity("rendering settings");
    world.addComponent<RenderingSettingsComponent>(renderingSettings, RenderingSettingsComponent());

    // background
    auto background = world.createEntity("sky");
    world.addComponent<RectMaterialComponent>(background, RectMaterialComponent(RectMaterial({0.0f, 0.0f, 4000.0f, 4000.0f})));
    auto quad = Mesh::quad({0.0f, 0.0f, 1.0f, 1.0f}, 0.9999f, {0.0f, 1.0f, 0.0f, 1.0f}, 1.0f, 1.0f);
    world.addComponent<MeshComponent>(background, MeshComponent(quad, "quad"));
    world.addComponent<RenderConfigComponent>(background, {"Skybox", false, Rendering::RenderLayer::BACKGROUND});
    world.addComponent<RenderRequestComponent>(background, RenderRequestComponent());
    world.addComponent<ViewportSizeMaterialComponent>(background, ViewportSizeMaterialComponent());

    // SPACESHIP
    // body
    auto ship = world.createEntity("ship_body");
    world.addComponent<PlayerComponent>(ship, {});
    auto& transform = world.addComponent<Transform>(ship, Transform());
    transform.position = {0.0f, 0.0f, 400.0f};
    world.addComponent<MeshRequestComponent>(ship, {Apple::resourcePath("ship_body_2.obj")});
    world.addComponent<RenderRequestComponent>(ship, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(ship, {"RUSTY_METAL", true, Rendering::RenderLayer::OPAQUE});
    auto& bodyWardMat = world.addComponent<WardMaterialComponent>(ship, WardMaterialComponent());
    bodyWardMat.metallic = 0.4f;
    bodyWardMat.roughness = 0.3f;
    auto& rb = world.addComponent<RigidBodyComponent>(ship, RigidBodyComponent(ColliderType::HULL));
    rb.mass = 1000.0f;
    rb.active = true;

    // reactors
    auto reactors = world.createEntity("ship_reactors");
    world.addComponent<Transform>(reactors, Transform());
    world.addComponent<MeshRequestComponent>(reactors, {Apple::resourcePath("ship_reactors_2.obj")});
    world.addComponent<RenderRequestComponent>(reactors, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(reactors, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    auto& reactorsWardMat = world.addComponent<WardMaterialComponent>(reactors, WardMaterialComponent());
    reactorsWardMat.metallic = 0.7f;
    reactorsWardMat.roughness = 0.3f;
    world.addComponent<ChildComponent>(reactors, {ship, {}});

    // cabin
    auto cabin = world.createEntity("cabin");
    world.addComponent<Transform>(cabin, Transform());
    world.addComponent<MeshRequestComponent>(cabin, {Apple::resourcePath("ship_cabin_2.obj")});
    world.addComponent<RenderRequestComponent>(cabin, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(cabin, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::TRANSPARENT});
    auto& cabinWardMat = world.addComponent<WardMaterialComponent>(cabin, WardMaterialComponent());
    cabinWardMat.metallic = 0.8f;
    cabinWardMat.roughness = 0.4f;
    cabinWardMat.alpha = 0.5f;
    world.addComponent<ChildComponent>(cabin, {ship, {}});

    // ambient light
    auto ambientLight = world.createEntity("ambient light");
    world.addComponent(ambientLight, AmbientLightComponent({0.1f, 0.1f, 0.1f}));

    auto lightScale = 0.145f;
    auto leftLightLocalPos = glm::vec3(-2.160f, 0.555f, 1.974f);
    // ship lights
    auto leftLight = world.createEntity("left ship light");
    auto& leftLightTransform = world.addComponent<Transform>(leftLight, Transform());
    leftLightTransform.scale = glm::vec3(lightScale, lightScale, lightScale);
    world.addComponent<MeshRequestComponent>(leftLight, {Apple::resourcePath("red_bulb.obj")});
    world.addComponent<RenderRequestComponent>(leftLight, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(leftLight, {"Unlit", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<PointLightComponent>(leftLight, PointLightComponent({{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.0f}}, 1.0f));
    auto& llChildComp = world.addComponent<ChildComponent>(leftLight, {ship, {}});
    llChildComp.localTransform.position = leftLightLocalPos;
    world.addComponent<UnlitMaterialComponent>(leftLight, {{1.0f, 0.0f, 0.0f, 1.0f}});

    auto leftFlare = world.createEntity("left_flare");
    world.addComponent<Transform>(leftFlare, Transform());
    world.addComponent<MeshComponent>(leftFlare, MeshComponent(Mesh::quad({-0.5f, -0.5f, 1.0f, 1.0f}, 0.1f, {1.0f, 0.5f, 0.5f, 1.0f}, 1.0f, 1.0f), "quad"));
    world.addComponent<RenderRequestComponent>(leftFlare, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(leftFlare, {"Billboard", false, Rendering::RenderLayer::TRANSPARENT});
    auto& leftFlareChildComp = world.addComponent<ChildComponent>(leftFlare, {ship, {}});
    leftFlareChildComp.localTransform.position = leftLightLocalPos;
    world.addComponent<TexturesRequestComponent>(leftFlare, TexturesRequestComponent({Apple::resourcePath("red_flare.png")}, {TextureType::Billboard}));
    world.addComponent<BillboardMaterialComponent>(leftFlare, {});
    auto& leftPC = world.addComponent<ParticleComponent>(leftFlare, ParticleComponent());
    leftPC.softness = 0.5f;

    auto rightLightLocalPos = glm::vec3(2.178f, 0.558f, 1.958f);
    auto rightLight = world.createEntity("right ship light");
    auto& rightLightTransform = world.addComponent<Transform>(rightLight, Transform());
    rightLightTransform.scale = glm::vec3(lightScale, lightScale, lightScale);
    world.addComponent<MeshRequestComponent>(rightLight, {Apple::resourcePath("red_bulb.obj")});
    world.addComponent<RenderRequestComponent>(rightLight, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(rightLight, {"Unlit", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<PointLightComponent>(rightLight, PointLightComponent({{0.0f, 0.0f, 0.0f, 1.0f}, {1.0f, 0.0f, 0.0f, 0.0f}}, 1.0f));
    auto& rlChildComp = world.addComponent<ChildComponent>(rightLight, {ship, {}});
    rlChildComp.localTransform.position = rightLightLocalPos;
    world.addComponent<UnlitMaterialComponent>(rightLight, {{1.0f, 0.0f, 0.0f, 1.0f}});

    auto rightFlare = world.createEntity("right_flare");
    world.addComponent<Transform>(rightFlare, Transform());
    world.addComponent<MeshComponent>(rightFlare, MeshComponent(Mesh::quad({-0.5f, -0.5f, 1.0f, 1.0f}, 0.1f, {1.0f, 0.5f, 0.5f, 1.0f}, 1.0f, 1.0f), "quad"));
    world.addComponent<RenderRequestComponent>(rightFlare, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(rightFlare, {"Billboard", false, Rendering::RenderLayer::TRANSPARENT});
    auto& rightFlareChildComp = world.addComponent<ChildComponent>(rightFlare, {ship, {}});
    rightFlareChildComp.localTransform.position = rightLightLocalPos;
    world.addComponent<TexturesRequestComponent>(rightFlare, TexturesRequestComponent({Apple::resourcePath("red_flare.png")}, {TextureType::Billboard}));
    world.addComponent<BillboardMaterialComponent>(rightFlare, {});
    auto& rightPC = world.addComponent<ParticleComponent>(rightFlare, ParticleComponent());
    rightPC.softness = 0.5f;

    auto flames = world.createEntity("flames");
    world.addComponent<Transform>(flames, Transform());
    world.addComponent<MeshRequestComponent>(flames, {Apple::resourcePath("ship_flame.obj")});
    world.addComponent<RenderRequestComponent>(flames, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(flames, {"Flame", true, Rendering::RenderLayer::TRANSPARENT});
    world.addComponent<ChildComponent>(flames, {ship, {}});

    auto cores = world.createEntity("cores");
    world.addComponent<Transform>(cores, Transform());
    world.addComponent<MeshRequestComponent>(cores, {Apple::resourcePath("ship_reactors_core_2.obj")});
    world.addComponent<RenderRequestComponent>(cores, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(cores, {"HotMetal", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<UnlitMaterialComponent>(cores, {{5.0f, 5.0, 5.0f, 1.0f}});
    world.addComponent<ChildComponent>(cores, {ship, {}});

    auto interior = world.createEntity("interior");
    world.addComponent<Transform>(interior, Transform());
    world.addComponent<MeshRequestComponent>(interior, {Apple::resourcePath("ship_interior_2.obj")});
    world.addComponent<RenderRequestComponent>(interior, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(interior, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<ChildComponent>(interior, {ship, {}});
    auto& interiorWardMaterial = world.addComponent<WardMaterialComponent>(interior, WardMaterialComponent());
    interiorWardMaterial.metallic = 0.0f;
    interiorWardMaterial.roughness = 0.6f;

    auto cabinFrame = world.createEntity("cabin frame");
    world.addComponent<Transform>(cabinFrame, Transform());
    world.addComponent<MeshRequestComponent>(cabinFrame, {Apple::resourcePath("ship_cabin_frame_2.obj")});
    world.addComponent<RenderRequestComponent>(cabinFrame, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(cabinFrame, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<ChildComponent>(cabinFrame, {ship, {}});
    auto& cabinFrameWardMaterial = world.addComponent<WardMaterialComponent>(cabinFrame, WardMaterialComponent());
    interiorWardMaterial.metallic = 0.7f;
    interiorWardMaterial.roughness = 0.6f;

    auto lightCages = world.createEntity("light cages");
    world.addComponent<Transform>(lightCages, Transform());
    world.addComponent<MeshRequestComponent>(lightCages, {Apple::resourcePath("ship_light_cages_2.obj")});
    world.addComponent<RenderRequestComponent>(lightCages, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(lightCages, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<ChildComponent>(lightCages, {ship, {}});
    auto& lightCagesWardMaterial = world.addComponent<WardMaterialComponent>(lightCages, WardMaterialComponent());
    interiorWardMaterial.metallic = 0.8f;
    interiorWardMaterial.roughness = 0.2f;

    auto globalMaterials = world.createEntity("global materials");
    world.addComponent<AtmosphereSettingsComponent>(globalMaterials, AtmosphereSettingsComponent());

    // sun
    auto sun = world.createEntity("sun");
    auto& dirLightComp = world.addComponent(sun, DirectionalLightComponent());
    dirLightComp.light.color = {8.0f, 8.0f, 8.0f, 1.0f};
    dirLightComp.light.direction = {0.9f, -1.0f, 0.8, 0.0f};
    dirLightComp.nPasses = 3;

    dirLightComp.settings[1].distance = 400.0f;
    dirLightComp.settings[1].orthoSize = 400.0f;
    dirLightComp.settings[1].farPlane = 800.0f;

    dirLightComp.settings[2].distance = 30.0f;
    dirLightComp.settings[2].orthoSize = 30.0f;
    dirLightComp.settings[2].farPlane = 60.0f;


    // MOUSE RAY
    auto mouseRay = world.createEntity("mouse ray");
    world.addComponent<MouseRay>(mouseRay, MouseRay());

    // CAMERA
    auto cameraID = world.createEntity("camera");
    world.addComponent<Transform>(cameraID, Transform());
    world.addComponent(cameraID, PointLightComponent({{0.0f, 0.0f, 2.0f, 1.0f}, {0.0f, 0.0, 0.0f, 1.0f}}));
    auto& cameraComponent = world.addComponent<CameraComponent>(cameraID, {std::make_shared<FPSCamera>()});
    auto camera = static_pointer_cast<FPSCamera>(cameraComponent.camera);

    // STATIC VARIABLES BEFORE LOOP
    bool isCursorActive = true;
    SDL_SetRelativeMouseMode(SDL_FALSE);

    // selected entity
    uint64_t selectedEntity = 0;

    SDL_Event event;
    auto running = true;

    float deltaTime = 0.0f;
    auto lastTime = SDL_GetPerformanceCounter();
    auto start = SDL_GetPerformanceCounter();

    float thurst = 0.0f;
    float heat = 0.0f;

    while (running)
    {
        // DRAW AXES
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {10000.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {0.0f, 10000.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10000.0f}, {0.0f, 0.0f, 1.0f, 1.0f});

        bool leftMouseButtonDown = false;

        // FPS COUNTER
        auto now = SDL_GetPerformanceCounter();

        // set time material
        ctx.renderer->setGlobalMaterial(getBytes(static_cast<float>(static_cast<double>(now - start) / static_cast<double>(SDL_GetPerformanceFrequency()))), TIME);
        deltaTime = static_cast<float>(static_cast<double>(now - lastTime) / static_cast<double>(SDL_GetPerformanceFrequency()));
        lastTime = now;

        // INPUT LOOP
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            if (isCursorActive)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (ImGui::GetIO().WantCaptureMouse) continue;
            }

            if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.scancode == SDL_SCANCODE_SPACE)
                {
                    isCursorActive = !isCursorActive;
                    SDL_SetRelativeMouseMode(isCursorActive ? SDL_FALSE : SDL_TRUE);
                }
            }

            if (!isCursorActive && event.type == SDL_MOUSEMOTION)
            {
                float sensitivity = 0.002f;
                camera->localPan(-static_cast<float>(event.motion.xrel) * sensitivity);
                camera->tilt(-static_cast<float>(event.motion.yrel) * sensitivity);
            }

            if (event.type == SDL_MOUSEBUTTONDOWN and event.button.button == SDL_BUTTON_LEFT)
            {
                leftMouseButtonDown = true;
            }

        }
        auto shipPosition = world.getComponent<Transform>(ship).position;

        // DEBUG OCTREE
        if (world.hasComponent<OctreeComponent>(planet))
        {
            auto& octreeComponent = world.getComponent<OctreeComponent>(planet);
            int depth;
            if (octreeComponent.octree.size() > 0)
            octreeComponent.debugPotential = gravity::potential::get_potential_from_octree(
                shipPosition, octreeComponent.octree, octreeComponent.min, octreeComponent.edge, &depth
            );
        }


        // update light center with ship position
        auto lightDirection = glm::normalize(glm::vec3(dirLightComp.light.direction));
        auto d = glm::dot(lightDirection, shipPosition + lightDirection * 250.0f);
        d = abs(d);

        dirLightComp.settings[0].distance = 100.0f + d;
        dirLightComp.settings[0].orthoSize = 150.0f;
        dirLightComp.settings[0].farPlane = 2.0f * (100.0f + d);
        dirLightComp.settings[0].center = shipPosition;

        ///////////////////////////////////////////////////////////////////////////////
        /////////////////////////////////// INPUT /////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        SDL_PumpEvents();
        int* length = nullptr;
        auto keyboardState = SDL_GetKeyboardState(length);

        auto actualCamSpeed = 10.0f;
        if (keyboardState[SDL_SCANCODE_X]) actualCamSpeed *= 10.0f;

        auto velocity = glm::vec2({0.0f, 0.0f});
        if (keyboardState[SDL_SCANCODE_LEFT]) velocity.x -= 1.0f;
        if (keyboardState[SDL_SCANCODE_RIGHT]) velocity.x += 1.0f;
        if (keyboardState[SDL_SCANCODE_UP]) velocity.y += 1.0f;
        if (keyboardState[SDL_SCANCODE_DOWN]) velocity.y -= 1.0f;
        if (glm::length(velocity) > 0.0f)
        {
            velocity = glm::normalize(velocity);
        }

        if (debugCamera)
        {
            auto fpsCamera = static_pointer_cast<FPSCamera>(camera);
            fpsCamera->advance(velocity.y * actualCamSpeed * deltaTime);
            fpsCamera->strafe(velocity.x * actualCamSpeed * deltaTime);
        }

        auto shipTransform = world.getComponent<Transform>(ship);

        // MOVE SHIP

        auto forwardSpeed = 40000.0f;
        auto lateralSpeed = 5000.0f;
        auto yawSpeed = 10000.0f;
        auto pitchSpeed = 10000.0f;

        // move camera with left stick

        auto xAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTX) / 32767.0f;
        auto yAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_LEFTY) / 32767.0f;

        if (abs(yAxis) >= 0.2)
        {
            camera->tilt(-yAxis * actualCamSpeed * deltaTime * 0.3f);
        }
        if (abs(xAxis) >= 0.2)
        {
            camera->pan(-xAxis * actualCamSpeed * deltaTime * 0.3f);
        }

        /*
        // MOVE CAMERA
        if (!debugCamera and world.hasComponent<Transform>(ship) and world.hasComponent<MeshComponent>(ship))
        {
            auto tc = static_pointer_cast<FPSCamera>(camera);
            auto worldPosition = world.getComponent<Transform>(ship).position;
            auto centerOfMass = world.getComponent<MeshComponent>(ship).mesh->centerOfMass();
            tc->setPosition(worldPosition + centerOfMass - 30.0f * tc->front() + 8.0f * tc->up());
        }*/

         // CAMERA ROLL
        auto rollSpeed = 1.0f;
        if (keyboardState[SDL_SCANCODE_U])
        {
            camera->roll(-deltaTime * rollSpeed);
        }
        if (keyboardState[SDL_SCANCODE_O])
        {
            camera->roll(deltaTime * rollSpeed);
        }

        if (world.hasComponent<RigidBodyComponent>(ship))
        {
            auto& rigidb = world.getComponent<RigidBodyComponent>(ship);
            if (auto body = rigidb.body)
            {
                body->setDamping(0.6f, 0.8f);

                if (
                    SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_UP)
                    or keyboardState[SDL_SCANCODE_UP] or keyboardState[SDL_SCANCODE_I]
                    )
                {
                    auto dir = shipTransform.front();
                    auto force = btVector3(dir.x, dir.y, dir.z) * forwardSpeed;
                    body->applyCentralForce(force);

                    // THURSTING
                    thurst += deltaTime * 2.0f;
                    thurst = clamp(thurst, 0.0f, 1.0f);

                    heat += deltaTime * 1.0f;
                    heat = clamp(heat, 0.0f, 1.0f);
                }
                else
                {
                    thurst -= deltaTime * 2.0f;
                    thurst = clamp(thurst, 0.0f, 1.0f);

                    heat -= deltaTime * 0.1f;
                    heat = clamp(heat, 0.0f, 1.0f);
                }
                if (
                    SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_DOWN)
                    or keyboardState[SDL_SCANCODE_DOWN] or keyboardState[SDL_SCANCODE_K]
                    )
                {
                    auto dir = -shipTransform.front();
                    auto force = btVector3(dir.x, dir.y, dir.z) * lateralSpeed;
                    body->applyCentralForce(force);
                }
                if (
                    SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_LEFT)
                    or keyboardState[SDL_SCANCODE_LEFT] or keyboardState[SDL_SCANCODE_J]
                    )
                {
                    auto dir = -shipTransform.right();
                    auto force = btVector3(dir.x, dir.y, dir.z) * lateralSpeed;
                    body->applyCentralForce(force);
                }
                if (
                    SDL_GameControllerGetButton(controller, SDL_CONTROLLER_BUTTON_DPAD_RIGHT)
                    or keyboardState[SDL_SCANCODE_RIGHT] or keyboardState[SDL_SCANCODE_L]
                    )
                {
                    auto dir = shipTransform.right();
                    auto force = btVector3(dir.x, dir.y, dir.z) * lateralSpeed;
                    body->applyCentralForce(force);
                }

                auto xAxis = SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTX) / 32767.0f;
                auto yAxis = -SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_RIGHTY) / 32767.0f;

                // set pitch around right vector
                if (abs(yAxis) > 0.2f)
                body->applyTorque(btVector3(-shipTransform.right().x * yAxis * pitchSpeed, -shipTransform.right().y * yAxis * pitchSpeed, -shipTransform.right().z * yAxis * pitchSpeed));
                // set yaw around up vector
                auto upAxis = shipTransform.up();
                if (abs(xAxis) > 0.2f)
                body->applyTorque(btVector3(upAxis.x * -xAxis * yawSpeed, upAxis.y * -xAxis * yawSpeed, upAxis.z * -xAxis * yawSpeed));

                if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERLEFT) / 32767.0f > 0.5f)
                {
                    // apply roll
                    auto rollAxis = shipTransform.front();
                    auto speed = 10000.0f;
                    body->applyTorque(btVector3(-rollAxis.x * speed, -rollAxis.y * speed, -rollAxis.z * speed));
                }
                else if (SDL_GameControllerGetAxis(controller, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) / 32767.0f > 0.5f)
                {
                    // apply roll
                    auto rollAxis = shipTransform.front();
                    auto speed = 10000.0f;
                    body->applyTorque(btVector3(rollAxis.x * speed, rollAxis.y * speed, rollAxis.z * speed));
                }

                // rotate body towards camera direction
                // 1. ALLINEAMENTO DIREZIONE (Forward)
                auto sFront = shipTransform.front();
                auto cFront = camera->front();
                btVector3 shipForward(sFront.x, sFront.y, sFront.z);
                btVector3 camForward(cFront.x, cFront.y, cFront.z);

                btVector3 errorForward = shipForward.cross(camForward);

                // 2. ALLINEAMENTO INCLINAZIONE (Up)
                auto sUp = shipTransform.up();
                auto cUp = camera->up();
                btVector3 shipUp(sUp.x, sUp.y, sUp.z);
                btVector3 camUp(cUp.x, cUp.y, cUp.z);

                btVector3 errorUp = shipUp.cross(camUp);

                // 3. COMBINAZIONE DEGLI ERRORI
                // Sommiamo i due vettori di errore. Bullet spingerà la nave a correggere
                // contemporaneamente il Pitch/Yaw (da errorForward) e il Roll (da errorUp).
                btVector3 totalError = errorForward + errorUp;

                // 4. APPLICAZIONE VELOCITÀ ANGOLARE
                float rotationSpeed = 0.7f; // Puoi alzarlo o abbassarlo per regolare la reattività
                body->setAngularVelocity(totalError * rotationSpeed);
            }
        }

        // set thurst material
        ctx.renderer->setGlobalMaterial(getBytes(thurst), THURST);
        ctx.renderer->setGlobalMaterial(getBytes(heat), HEAT);


          ///////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////// UI // /////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        _renderer->setDebugUICallback([this, &world, &selectedEntity, &ctx, inspector, deltaTime]
        {
            ImGui::Begin("General Info");
            ImGui::Text("Delta Time: %.3f ms", deltaTime * 1000.0f);
            ImGui::End();

            ImGui::Begin("Entities");
            ImGui::Text("Selected Entity: %lu", selectedEntity);
            for (uint64_t e : world.getEntities())
            {
                bool isSelected = (e == selectedEntity);

                // Etichetta unica (ImGui vuole ID univoci)
                std::string label = "Entity " + std::to_string(e) + ": " + world.getComponent<NameComponent>(e).name;

                if (ImGui::Selectable(label.c_str(), isSelected))
                {
                    selectedEntity = e;
                }
            }
            ImGui::End();


            // inspector
            ImGui::Begin("Inspector");
            if (selectedEntity != 0)
            {
                auto label = "Entity " + std::to_string(selectedEntity) + ": " + world.getComponent<NameComponent>(selectedEntity).name;
                ImGui::Text(label.c_str());
                inspector.entityInspector(world, ctx, selectedEntity);
            }
            ImGui::End();
        });

          ///////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////// SYSTEMS ///////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        systemManager.update(world, ctx, deltaTime);

        //std::cout << "FPS: " << 1.0f / deltaTime << std::endl;

    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from RTGPApp::run()" << std::endl;
}
