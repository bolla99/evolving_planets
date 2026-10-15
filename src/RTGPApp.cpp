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

#include "Scripts//Scripts.hpp"

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

    // CREATE WORLD AND CONTEXT
    auto world = World();
    auto input = Core::Input();
    auto ctx = Context{_renderer.get(), _window, &_assetManager, &input};
    auto inspector = ComponentsInspector::factory();


    // SYSTEMS MANAGER
    auto systemManager = SystemsManager();

    // LOADING AND REGISTRATION
    systemManager.addSystem<ResetRigidBodies>();
    systemManager.addSystem<PlanetGeneratorSystem>();
    systemManager.addSystem<MeshLoadingSystem>();
    systemManager.addSystem<TextureLoadingSystem>();
    systemManager.addSystem<RenderRegistrationSystem>();
    systemManager.addSystem<BoundingSphereRequestSystem>();
    systemManager.addSystem<UpdateBVH>();

    // GAMEPLAY SYSTEMS
    //systemManager.addSystem<MouseRaySystem>();
    //systemManager.addSystem<MouseIntersectionSystem>();

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

    // GAMEPLAY SYSTEMS - PRE PHYSICS
    systemManager.addSystem<PlayerControllerSystem>();
    systemManager.addSystem<BeforePhysicsScriptSystem>();

    // UPDATE PHYSICS
    systemManager.addSystem<PhysicsSystem>();

    // GAMEPLEY PHYSICS - POST PHYSICS
    systemManager.addSystem<UpdateCameraPositionSystem>();
    systemManager.addSystem<UpdateRenderingSettings>();

    // MOVE CHILDREN: apply child system
    systemManager.addSystem<MoveChildren>();
    // MOVE BILLBOARDS
    systemManager.addSystem<MoveBillboards>();
    systemManager.addSystem<UpdateBillboardMaterialsSystem>();

    // APPLY TRANSFORM TO GRAPHICS
    systemManager.addSystem<TransformSystem>();

    // UPDATE LIGHTS: set lighting materials for rendering
    systemManager.addSystem<LightAndShadowSystem>();

    // Apply Rendering Settings
    systemManager.addSystem<UpdateRenderingSettings>();

    // RENDER UPDATE
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
    transform.position = {0.0f, 0.0f, 600.0f};
    world.addComponent<MeshRequestComponent>(ship, {Apple::resourcePath("ship_body_2.obj")});
    world.addComponent<RenderRequestComponent>(ship, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(ship, {"RUSTY_METAL", true, Rendering::RenderLayer::OPAQUE});
    auto& bodyWardMat = world.addComponent<WardMaterialComponent>(ship, WardMaterialComponent());
    bodyWardMat.mat.metallic = 0.4f;
    bodyWardMat.mat.roughness = 0.3f;
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
    reactorsWardMat.mat.metallic = 0.9f;
    reactorsWardMat.mat.roughness = 0.2f;
    world.addComponent<ChildComponent>(reactors, {ship, {}});

    // cabin
    auto cabin = world.createEntity("cabin");
    world.addComponent<Transform>(cabin, Transform());
    world.addComponent<MeshRequestComponent>(cabin, {Apple::resourcePath("ship_cabin_2.obj")});
    world.addComponent<RenderRequestComponent>(cabin, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(cabin, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::TRANSPARENT});
    auto& cabinWardMat = world.addComponent<WardMaterialComponent>(cabin, WardMaterialComponent());
    cabinWardMat.mat.metallic = 0.8f;
    cabinWardMat.mat.roughness = 0.4f;
    cabinWardMat.mat.alpha = 0.5f;
    world.addComponent<ChildComponent>(cabin, {ship, {}});

    // ambient light
    auto ambientLight = world.createEntity("ambient light");
    world.addComponent(ambientLight, AmbientLightComponent({0.0f, 0.0f, 0.0f}));

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
    world.addComponent<RenderConfigComponent>(flames, {"Flame", false, Rendering::RenderLayer::TRANSPARENT});
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
    interiorWardMaterial.mat.metallic = 0.0f;
    interiorWardMaterial.mat.roughness = 0.6f;

    auto cabinFrame = world.createEntity("cabin frame");
    world.addComponent<Transform>(cabinFrame, Transform());
    world.addComponent<MeshRequestComponent>(cabinFrame, {Apple::resourcePath("ship_cabin_frame_2.obj")});
    world.addComponent<RenderRequestComponent>(cabinFrame, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(cabinFrame, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<ChildComponent>(cabinFrame, {ship, {}});
    auto& cabinFrameWardMaterial = world.addComponent<WardMaterialComponent>(cabinFrame, WardMaterialComponent());
    interiorWardMaterial.mat.metallic = 0.7f;
    interiorWardMaterial.mat.roughness = 0.6f;

    auto lightCages = world.createEntity("light cages");
    world.addComponent<Transform>(lightCages, Transform());
    world.addComponent<MeshRequestComponent>(lightCages, {Apple::resourcePath("ship_light_cages_2.obj")});
    world.addComponent<RenderRequestComponent>(lightCages, RenderRequestComponent());
    world.addComponent<RenderConfigComponent>(lightCages, {"VCWARD_FULLSHADOW", true, Rendering::RenderLayer::OPAQUE});
    world.addComponent<ChildComponent>(lightCages, {ship, {}});
    auto& lightCagesWardMaterial = world.addComponent<WardMaterialComponent>(lightCages, WardMaterialComponent());
    interiorWardMaterial.mat.metallic = 0.8f;
    interiorWardMaterial.mat.roughness = 0.2f;

    auto globalMaterials = world.createEntity("global materials");
    world.addComponent<AtmosphereSettingsComponent>(globalMaterials, AtmosphereSettingsComponent());
    world.addComponent<GravitySettingsComponent>(globalMaterials, GravitySettingsComponent());

    // sun
    auto sun = world.createEntity("sun");
    auto& dirLightComp = world.addComponent(sun, DirectionalLightComponent());
    world.addComponent<ScriptComponent>(sun, ScriptComponent::before(sunScriptB));
    dirLightComp.light.color = {8.0f, 8.0f, 8.0f, 1.0f};
    dirLightComp.light.direction = {0.9f, -1.0f, 0.8, 0.0f};
    dirLightComp.nPasses = 4;


    // CAMERA
    auto cameraID = world.createEntity("camera");
    world.addComponent<Transform>(cameraID, Transform());
    world.addComponent<CameraComponent>(cameraID, {std::make_shared<FPSCamera>()});
    world.addComponent<TagComponent>(cameraID, {{MAIN_CAMERA}});
    world.addComponent<ScriptComponent>(cameraID, ScriptComponent::before(mainCameraScriptB));

    // DEBUG CAMERA
    auto debugCameraID = world.createEntity("debug camera");
    world.addComponent<Transform>(debugCameraID, Transform());
    world.addComponent<CameraComponent>(debugCameraID, {std::make_shared<FPSCamera>()});
    world.addComponent<TagComponent>(debugCameraID, {{DEBUG_CAMERA}});
    world.addComponent<ScriptComponent>(debugCameraID, ScriptComponent::before(debugCameraScriptB));

    // STATIC VARIABLES BEFORE LOOP
    SDL_SetRelativeMouseMode(SDL_FALSE);

    // selected entity
    uint64_t selectedEntity = 0;

    SDL_Event event;
    auto running = true;

    float deltaTime = 0.0f;
    auto lastTime = SDL_GetPerformanceCounter();
    auto start = SDL_GetPerformanceCounter();

    std::shared_ptr<FPSCamera> camera;

    while (running)
    {
        input.update();

        auto activeCameraID = world.getActiveCamera();
        if (activeCameraID != 0)
        {
            camera = static_pointer_cast<FPSCamera>(world.getComponent<CameraComponent>(activeCameraID).camera);
        }

        // DRAW AXES
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {10000.0f, 0.0f, 0.0f}, {1.0f, 0.0f, 0.0f, 1.0f});
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {0.0f, 10000.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 1.0f});
        //ctx.renderer->debugDrawLine({0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10000.0f}, {0.0f, 0.0f, 1.0f, 1.0f});

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
            if (input.isCursorActive)
            {
                ImGui_ImplSDL2_ProcessEvent(&event);
                if (ImGui::GetIO().WantCaptureMouse) continue;
            }
        }

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
