//
// Created by Giovanni Bollati on 29/01/26.
//

#include <SDL.h>
#include <RTGPApp.hpp>

#include "AssimpMeshLoader.hpp"
#include "imgui.h"
#include "imgui_impl_sdl2.h"
#include "../include/Engine/Pool.hpp"
#include "../include/Engine/Storage.hpp"
#include "TrackballCamera.hpp"
#include "../include/Engine/World.hpp"
#include "Apple/Util.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "../include/Engine/Components.hpp"
#include <Engine/Systems.hpp>

void RTGPApp::init()
{
    // RENDER AXES
    std::vector<glm::vec3> x = {{0.0f, 0.0f, 0.0f}, {1000.0f, 0.0f, 0.0f}};
    std::vector<glm::vec3> y = {{0.0f, 0.0f, 0.0f}, {0.0f, 1000.0f, 0.0f}};
    std::vector<glm::vec3> z = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1000.0f}};
    auto xMesh = Mesh::fromPolygon(x, {1.0f, 0.0f, 0.0f, 1.0f}, false);
    auto yMesh = Mesh::fromPolygon(y, {0.0f, 1.0f, 0.0f, 1.0f}, false);
    auto zMesh = Mesh::fromPolygon(z, {0.0f, 0.0f, 1.0f, 1.0f}, false);
    _renderer->addRenderable(*xMesh, Rendering::psoConfigs.at("curve"), {}, Rendering::RenderLayer::OPAQUE);
    _renderer->addRenderable(*yMesh, Rendering::psoConfigs.at("curve"), {}, Rendering::RenderLayer::OPAQUE);
    _renderer->addRenderable(*zMesh, Rendering::psoConfigs.at("curve"), {}, Rendering::RenderLayer::OPAQUE);
}

void RTGPApp::run()
{
    // CREATE WORLD
    auto world = World();

    // LIGHT ENTITIES
    auto pointLight = world.createEntity("point light");
    world.addComponent(pointLight, PointLightComponent({{0.0f, 0.0f, 2.0f, 1.0f}, {0.5f, 0.1f, 0.1f, 1.0f}}));
    world.addComponent(pointLight, Transform({.position = {0.0f, 0.0f, 2.0f}}));

    auto directionalLight = world.createEntity("sun");
    world.addComponent(directionalLight, DirectionalLightComponent());
    world.getComponent<DirectionalLightComponent>(directionalLight).light.color = {0.2f, 0.2f, 0.2f, 1.0f};


    // MOUSE RAY ENTITY
    auto mouseRay = world.createEntity("mouse ray");
    world.addComponent<MouseRay>(mouseRay, MouseRay());

    // CAMERA ENTITY
    auto cameraID = world.createEntity("camera");
    world.addComponent<CameraComponent>(cameraID, {std::make_shared<TrackballCamera>()});
    static_pointer_cast<TrackballCamera>(world.getComponent<CameraComponent>(world.query<CameraComponent>()[0]).camera)->zoom(5.0f);
    auto& camera = *world.getComponent<CameraComponent>(cameraID).camera;

    auto cameraSpeed = 100.0f;

    // create mesh loader
    std::unique_ptr<IMeshLoader> meshLoader = std::make_unique<AssimpMeshLoader>();

    // selected entity
    uint64_t selectedEntity = 0;

    // MONKEY ENTITY
    auto monkey = world.createEntity("monkey");
    world.addComponent<MeshComponent>(monkey, {meshLoader->loadMesh(Apple::resourcePath("monkey.obj"))});
    world.addComponent<RenderConfigComponent>(monkey, {"VCPHONG_WITH_TINT", Rendering::RenderLayer::OPAQUE});
    world.addComponent<Transform>(monkey, Transform());
    world.addComponent<MouseRayIntersectionComponent>(monkey, MouseRayIntersectionComponent());

    // debug ball
    auto debugBall = world.createEntity("debug ball");
    world.addComponent<Transform>(debugBall, Transform());
    world.getComponent<Transform>(debugBall).scale = {0.05f, 0.05f, 0.05f};
    auto debugBallMesh = meshLoader->loadMesh(Apple::resourcePath("ico.fbx"));
    world.addComponent<MeshComponent>(debugBall, {debugBallMesh});
    auto debugBallRenderableID = _renderer->addRenderable(
        *debugBallMesh, Rendering::psoConfigs.at("VCPHONG"), {}, Rendering::RenderLayer::OPAQUE
        );
    world.addComponent<RenderableComponent>(debugBall, {debugBallRenderableID});

    SDL_Event event;
    auto running = true;

    float deltaTime = 0.0f;
    auto lastTime = SDL_GetPerformanceCounter();

    float mouseDX = 0.0f;
    float mouseDY = 0.0f;

    while (running)
    {
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
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (ImGui::GetIO().WantCaptureMouse) continue;


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

            auto velocity = glm::vec2(0.0f);
            auto speed = 5.0f;
            if (keyboardState[SDL_SCANCODE_LEFT]) velocity.x -= 1.0f;
            if (keyboardState[SDL_SCANCODE_RIGHT]) velocity.x += 1.0f;
            if (keyboardState[SDL_SCANCODE_UP]) velocity.y += 1.0f;
            if (keyboardState[SDL_SCANCODE_DOWN]) velocity.y -= 1.0f;
            velocity = glm::normalize(velocity);
            camera.advance(velocity.y * speed * deltaTime);
            camera.strafe(velocity.x * speed * deltaTime);


            //SDL_GetRelativeMouseState(&mouseDX, &mouseDY);
            // apply accumulated mouse movement
            camera.pan(-mouseDX * 0.005f * cameraSpeed);
            camera.tilt(-mouseDY * 0.005f * cameraSpeed);
            mouseDX = 0.0;
            mouseDY = 0.0;
        }

        world.getComponent<DirectionalLightComponent>(directionalLight).light.direction = glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);

          ///////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////// UI // /////////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        _renderer->setDebugUICallback([this, &world, monkey, &camera, &debugBall, &selectedEntity, leftMouseButtonDown]
        {
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
                world.entityInspector(selectedEntity);
            }
            ImGui::End();

            /*
            ImGui::Begin("Main");

            // get monkey math
            if (world.hasComponent<RenderableComponent>(monkey))
            {
                auto monkeyMat = _renderer->getMaterial<MatVCPHONG>(world.getComponent<RenderableComponent>(monkey).id);
                static glm::vec4 monkeyTint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
                ImGui::ColorEdit4("Monkey Tint", glm::value_ptr((monkeyTint)));
                monkeyMat->data.addTint = 1;
                monkeyMat->data.tint = monkeyTint;
            }

            auto mouseIntersectable = world.query<MouseRayIntersectionComponent>();
            if (leftMouseButtonDown) selectedEntity = 0;
            for (auto e : mouseIntersectable)
            {
                if (world.getComponent<MouseRayIntersectionComponent>(e).intersected)
                {
                    auto intersection = world.getComponent<MouseRayIntersectionComponent>(e).intersection;
                    world.getComponent<Transform>(debugBall).position = intersection;
                    if (leftMouseButtonDown and e != selectedEntity)
                        selectedEntity = e;
                }
            }
            world.getComponent<Transform>(monkey).ui();
            ImGui::End();
            */


        });

          ///////////////////////////////////////////////////////////////////////////////
         /////////////////////////////////// SYSTEMS ///////////////////////////////////
        ///////////////////////////////////////////////////////////////////////////////
        auto ctx = Context{_renderer.get(), _window};
        // apply transform to renderable entities
        TransformSystem().update(world, ctx, deltaTime);
        // set mouse ray according to camera
        MouseRaySystem().update(world, ctx, deltaTime);
        // pick renderable entities and register them to the renderer
        RenderRegistrationSystem().update(world, ctx, deltaTime);
        // update lights according to transform
        UpdateLightsWithTransformSystem().update(world, ctx, deltaTime);
        // send lights to rendering system
        SetLightsSystem().update(world, ctx, deltaTime);
        // register intersections with the mouse ray
        MouseIntersectionSystem().update(world, ctx, deltaTime);

        // RENDERING UPDATE
        _renderer->update(camera.getViewMatrix());
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from RTGPApp::run()" << std::endl;
}
