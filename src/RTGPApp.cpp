//
// Created by Giovanni Bollati on 29/01/26.
//

#include <SDL.h>
#include <RTGPApp.hpp>

#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "Engine/ECS/Pool.hpp"
#include "Engine/ECS/Storage.hpp"
#include "TrackballCamera.hpp"
#include "Engine/ECS/World.hpp"
#include "Apple/Util.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "../include/Engine/ECS/Components.hpp"
#include <Engine/ECS/Systems.hpp>

#include "timer.hpp"
#include "../include/Engine/ECS/SystemsManager.hpp"
#include "Engine/UI/ComponentsInspector.hpp"

#include <btBulletCollisionCommon.h>
#include <btBulletDynamicsCommon.h>

#include "Engine/ECS/Systems/PhysicsSystem.hpp"
#include "Engine/ECS/Systems/PlanetSystem.hpp"

void RTGPApp::init()
{}

void RTGPApp::run()
{
    // INIT BULLET
    auto* collisionConfiguration = new btDefaultCollisionConfiguration();
    auto dispatcher = new btCollisionDispatcher(collisionConfiguration);
    auto overlappingPairCache = new btDbvtBroadphase();
    auto solver = new btSequentialImpulseConstraintSolver();

    auto* dynamicsWorld = new btDiscreteDynamicsWorld(dispatcher, overlappingPairCache, solver, collisionConfiguration);
    dynamicsWorld->setGravity(btVector3(0, -9.81f, 0));


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
    // GAMEPLAY SYSTEMS
    systemManager.addSystem<MouseRaySystem>();
    systemManager.addSystem<MouseIntersectionSystem>();
    // UPDATE MATERIALS
    systemManager.addSystem<PlanetMaterialSystem>();
    systemManager.addSystem<UpdateWardMaterialSystem>();
    systemManager.addSystem<RectMaterialComponentSystem>();
    systemManager.addSystem<ViewportSizeMaterialSystem>();
    // UPDATE PHYSICS
    systemManager.addSystem<PhysicsSystem>();
    // APPLY TRANSFORM TO GRAPHICS
    systemManager.addSystem<TransformSystem>();
    systemManager.addSystem<UpdateCameraTransformSystem>();
    // UPDATE LIGHTS
    systemManager.addSystem<LightAndShadowSystem>();
    // GRAPHICS UPDATE
    systemManager.addSystem<RendererUpdateSystem>();

    // ENTITIES
    // RENDER AXES
    std::vector<glm::vec3> x = {{0.0f, 0.0f, 0.0f}, {10000.0f, 0.0f, 0.0f}};
    std::vector<glm::vec3> y = {{0.0f, 0.0f, 0.0f}, {0.0f, 10000.0f, 0.0f}};
    std::vector<glm::vec3> z = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10000.0f}};
    auto xMesh = Mesh::fromPolygon(x, {1.0f, 0.0f, 0.0f, 1.0f}, false);
    auto yMesh = Mesh::fromPolygon(y, {0.0f, 1.0f, 0.0f, 1.0f}, false);
    auto zMesh = Mesh::fromPolygon(z, {0.0f, 0.0f, 1.0f, 1.0f}, false);

    auto viewport = world.createEntity("viewport");
    world.addComponent<ViewportComponent>(viewport, ViewportComponent());

    auto planet = world.createEntity("planet");
    world.addComponent<PlanetConfigComponent>(planet, PlanetConfigComponent());
    world.addComponent<Transform>(planet, Transform());
    world.addComponent<PlanetInfoComponent>(planet, PlanetInfoComponent());

    // background
    auto background = world.createEntity("sky");
    world.addComponent<RectMaterialComponent>(background, RectMaterialComponent(RectMaterial({0.0f, 0.0f, 4000.0f, 4000.0f})));
    auto quad = Mesh::quad({0.0f, 0.0f, 1.0f, 1.0f}, 0.9999f, {0.0f, 1.0f, 0.0f, 1.0f}, 1.0f, 1.0f);
    world.addComponent<MeshComponent>(background, MeshComponent(quad, "quad"));
    world.addComponent<RenderConfigComponent>(background, {"Skybox", false, Rendering::RenderLayer::BACKGROUND});
    world.addComponent<RenderRequestComponent>(background, RenderRequestComponent());
    world.addComponent<ViewportSizeMaterialComponent>(background, ViewportSizeMaterialComponent());

    /*
    // LIGHT ENTITIES
    auto pointLight1 = world.createEntity("point light 1");
    world.addComponent(pointLight1, PointLightComponent({{0.0f, 0.0f, 2.0f, 1.0f}, {0.5f, 0.1f, 0.1f, 1.0f}}));
    world.addComponent(pointLight1, Transform({.position = {-2.0f, 0.0f, 2.0f}, .scale = {0.1f, 0.1f, 0.1f}, }));
    world.addComponent<MeshRequestComponent>(pointLight1, {Apple::resourcePath("ico.fbx")});
    world.addComponent<RenderRequestComponent>(pointLight1, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(pointLight1, {"VCPHONG", true, Rendering::RenderLayer::OPAQUE});

    auto pointLight2 = world.createEntity("point light 2");
    world.addComponent(pointLight2, PointLightComponent({{0.0f, 0.0f, 2.0f, 1.0f}, {0.5f, 0.1f, 0.1f, 1.0f}}));
    world.addComponent(pointLight2, Transform({.position = {2.0f, 0.0f, 2.0f}, .scale = {0.1f, 0.1f, 0.1f}, }));
    world.addComponent<MeshRequestComponent>(pointLight2, {Apple::resourcePath("ico.fbx")});
    world.addComponent<RenderRequestComponent>(pointLight2, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(pointLight2, {"VCPHONG", true, Rendering::RenderLayer::OPAQUE});
    */

    auto ambientLight = world.createEntity("ambient light");
    world.addComponent(ambientLight, AmbientLightComponent({0.1f, 0.1f, 0.1f}));

    auto sun1 = world.createEntity("sun1");
    world.addComponent(sun1, DirectionalLightComponent());
    world.getComponent<DirectionalLightComponent>(sun1).light.color = {8.0f, 8.0f, 8.0f, 1.0f};
    world.getComponent<DirectionalLightComponent>(sun1).light.direction = {0.9f, -1.0f, 0.8, 0.0f};

    /*
    auto sun2 = world.createEntity("sun2");
    world.addComponent(sun2, DirectionalLightComponent());
    world.getComponent<DirectionalLightComponent>(sun2).light.color = {1.0f, 1.0f, 1.0f, 1.0f};
    world.getComponent<DirectionalLightComponent>(sun2).light.direction = {-0.9f, -1.0f, 0.8, 0.0f};
    */

    // MOUSE RAY ENTITY
    auto mouseRay = world.createEntity("mouse ray");
    world.addComponent<MouseRay>(mouseRay, MouseRay());

    // CAMERA ENTITY
    auto cameraID = world.createEntity("camera");
    world.addComponent<Transform>(cameraID, Transform());
    world.addComponent(cameraID, PointLightComponent({{0.0f, 0.0f, 2.0f, 1.0f}, {10.0f, 10.0, 10.0f, 1.0f}}));
    auto& cameraComponent = world.addComponent<CameraComponent>(cameraID, {std::make_shared<FPSCamera>()});
    auto camera = cameraComponent.camera;
    camera->position = {0.0f, 0.0f, 550.0f};

    auto cameraSpeed = 5.0f;
    bool isCursorActive = true;
    SDL_SetRelativeMouseMode(SDL_FALSE);

    // selected entity
    uint64_t selectedEntity = 0;

    /*
    auto ball1 = world.createEntity("ball");
    world.addComponent<MeshRequestComponent>(ball1, {Apple::resourcePath("cagnaccio1.obj")});
    world.addComponent<RenderRequestComponent>(ball1, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(ball1, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    auto& transform = world.addComponent<Transform>(ball1, Transform());
    transform.position = {0.0f, 0.0f, 0.0f};
    world.addComponent<WardMaterialComponent>(ball1, WardMaterialComponent());
    auto& rb = world.addComponent<RigidBodyComponent>(ball1, RigidBodyComponent(ColliderType::SPHERE));
    rb.mass = 10.0f;
    rb.bounciness = 0.0f;
    rb.friction = 0.3f;
    rb.radius = 1.0f;
    */

    /*
    auto plane = world.createEntity("plane");
    world.addComponent<MeshRequestComponent>(plane, {Apple::resourcePath("plane.obj")});
    world.addComponent<RenderRequestComponent>(plane, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(plane, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<Transform>(plane, Transform());
    world.addComponent<WardMaterialComponent>(plane, WardMaterialComponent());
    world.addComponent<RigidBodyComponent>(plane, RigidBodyComponent(ColliderType::BOX));
    auto& rb2 = world.getComponent<RigidBodyComponent>(plane);
    rb2.mass = 0.0f;
    rb2.boxHalfExtents = {10.0f, 0.3f, 10.0f};
    rb2.isKinematic = true;
    rb2.friction = 0.3f;
    */


    SDL_Event event;
    auto running = true;

    float deltaTime = 0.0f;
    auto lastTime = SDL_GetPerformanceCounter();

    float mouseDX = 0.0f;
    float mouseDY = 0.0f;

    while (running)
    {
        // DRAW AXES
        ctx.renderer->iDraw(*xMesh, {}, "curve", {});
        ctx.renderer->iDraw(*yMesh, {}, "curve", {});
        ctx.renderer->iDraw(*zMesh, {}, "curve", {});

        bool leftMouseButtonDown = false;

        auto now = SDL_GetPerformanceCounter();
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
                camera->pan(-static_cast<float>(event.motion.xrel) * sensitivity);
                camera->tilt(-static_cast<float>(event.motion.yrel) * sensitivity);
            }

            if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)))
            {
                mouseDX += static_cast<float>(event.motion.xrel);
                mouseDY += static_cast<float>(event.motion.yrel);
            } else if (event.type == SDL_MOUSEBUTTONDOWN and event.button.button == SDL_BUTTON_LEFT)
            {
                leftMouseButtonDown = true;
            }

        }
          ///////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////// INPUT /////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        {
            SDL_PumpEvents();
            int* length = nullptr;
            auto keyboardState = SDL_GetKeyboardState(length);

            auto actualCamSpeed = cameraSpeed;
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
            camera->advance(velocity.y * actualCamSpeed * deltaTime);
            camera->strafe(velocity.x * actualCamSpeed * deltaTime);

            static auto angle_z = 0.0f;
            static auto angle_x = 0.0f;
            /*
            if (keyboardState[SDL_SCANCODE_LEFT]) angle_z += deltaTime * 20.0f;
            if (keyboardState[SDL_SCANCODE_RIGHT]) angle_z -= deltaTime * 20.0f;
            if (keyboardState[SDL_SCANCODE_UP]) angle_x -= deltaTime * 20.0f;
            if (keyboardState[SDL_SCANCODE_DOWN]) angle_x += deltaTime * 20.0f;
            */
            //std::cout << "angle_z: " << angle_z << ", angle_x: " << angle_x << std::endl;

            /*
            auto& planeTransform = world.getComponent<Transform>(plane);
            planeTransform.rotation = glm::rotate({1.0f, 0.0f, 0.0f, 0.0f}, glm::radians(angle_z), glm::vec3(0.0f, 0.0f, 1.0f));
            planeTransform.rotation = glm::rotate(planeTransform.rotation, glm::radians(angle_x), glm::vec3(1.0f, 0.0f, 0.0f));
            */



            //SDL_GetRelativeMouseState(&mouseDX, &mouseDY);
            // apply accumulated mouse movement
            mouseDX = 0.0;
            mouseDY = 0.0;
        }

        //world.getComponent<DirectionalLightComponent>(directionalLight).light.direction = glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

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
