
//
// Created by Giovanni Bollati on 06/03/25.
//

#include "App.hpp"

#include <thread>
#include <atomic>
#include <future>

#include <iostream>
#include <stdexcept>
#include <SDL2/SDL.h>
#include <Rendering/Metal/Renderer.hpp>

#include <AssimpMeshLoader.hpp>
#include <Rendering/Metal/RendererFactory.hpp>

#include <Apple/Util.hpp>
#include "TrackballCamera.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <Rendering/UI/UIManager.hpp>
#include <Rendering/UI/UIRenderer.hpp>
#include <random>

#include <imgui.h>

#include "BSpline.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_metal.h"
#include "Planet.hpp"
#include <gravity.hpp>
#include <GravityAdapter.hpp>
#include <ctime>
#include <cstdlib>

#include "genetic.hpp"

// app constructor initialize sdl, create a window and try to
// create a renderer from the custom class Renderer, which is in turn responsible
// for the acquisition of the sdl renderer, besides the metal structures.
App::App(
    const int width,
    const int height
    ) : _camera(std::move(std::make_unique<TrackballCamera>()))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    if (SDL_Init(SDL_INIT_VIDEO) < 0)
    {
        throw std::runtime_error(SDL_GetError());
    }

    // Explicitly disable high DPI mode to prevent resolution switching
    SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
    // Force scaling to be handled by the OS instead of the app
    SDL_SetHint(SDL_HINT_VIDEO_ALLOW_SCREENSAVER, "1");
    // Additional hints for better control on macOS
    SDL_SetHint(SDL_HINT_MAC_CTRL_CLICK_EMULATE_RIGHT_CLICK, "1");
    SDL_SetHint(SDL_HINT_VIDEO_MINIMIZE_ON_FOCUS_LOSS, "0");

    _window = SDL_CreateWindow(
        "App",
        SDL_WINDOWPOS_CENTERED,
        SDL_WINDOWPOS_CENTERED,
        width,
        height,
        SDL_WINDOW_METAL | SDL_WINDOW_RESIZABLE
        );
    if (!_window)
    {
        SDL_Quit();
        throw std::runtime_error(SDL_GetError());
    }

    ImGui_ImplSDL2_InitForMetal(_window);

    try
    {
        auto rendererFactory = std::make_unique<Rendering::Metal::RendererFactory>();
        _renderer = std::move(rendererFactory->createRenderer(_window));
    } catch (const std::exception& e)
    {
        SDL_Quit();
        std::cerr << e.what() << std::endl;
    }
}

/*
 * sdl and window cleanup by custom sdl functions
*/
App::~App()
{
    std::cout << "app::~app()" << std::endl;
    std::cout << "Calling SDL_DestroyWindow()" << std::endl;
    SDL_DestroyWindow(_window);
    std::cout << "SDL_DestroyWindow call ended" << std::endl;
    std::cout << "Calling SDL_Quit()" << std::endl;
    SDL_Quit();
    std::cout << "SDL_Quit call ended" << std::endl;
}
App& App::init()
{
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::cout << "app::init()" << std::endl;
    int w, h;
    SDL_GetWindowSize(_window, &w, &h);
    SDL_Log("WINDOW SIZE: %d x %d", w, h);
    SDL_GetWindowSizeInPixels(_window, &w, &h);
    SDL_Log("WINDOW SIZE in PIXELS: %d x %d", w, h);

    // load a mesh from file and add it to the renderer
    auto meshLoader = std::make_unique<AssimpMeshLoader>();

    try
    {
        std::shared_ptr<Mesh> mesh;
        try
        {
            mesh = meshLoader->loadMesh(Apple::resourcePath("textured_cube.fbx"));
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading mesh: " << e.what() << std::endl;
        }
        try
        {
            auto texture = Texture::fromFile(Apple::resourcePath("cube_texture.png"));
            /*_renderer->addRenderable(
                *mesh,
                Rendering::psoConfigs.at("TexturePHONG"), {texture});*/
        }
        catch (const std::exception& e)
        {
            std::cerr << "Error loading texture: " << e.what() << std::endl;
            std::terminate();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error adding renderable: " << e.what() << std::endl;
    }


    _renderer->addDirectionalLight(
        {
            {1.0f, -1.0f, 0.0f, 0.0f},
            {0.3f, 0.3f, 0.3f, 1.0f}
        });
    _renderer->addPointLight({
            {1.0f, -1.0f, 1.0f, 1.0f},
            {0.05f, 0.05f, 0.2f, 1.0f}
        });

    _renderer->addPointLight({
            {-1.0f, 1.0f, 1.0f, 1.0f},
            {0.1f, 0.1f, 0.2f, 1.0f}
        });

    _renderer->setAmbientGlobalLight({0.2f, 0.2f, 0.2f, 1.0f});

    static_cast<TrackballCamera*>(_camera.get())->zoom(5.0f);

    return *this;
}

// sdl loop: input management and renderer update
// right now input management is decoupled from the renderer, which means
// that the renderer is not aware of the input events, but input events can
// refer to the renderer, for example to change the state of a renderable object.
App& App::run()
{
    auto meshLoader = std::make_unique<AssimpMeshLoader>();
    auto controlPointMesh = meshLoader->loadMesh(Apple::resourcePath("ico.fbx"));

    std::vector<uint64_t> parallelsAndMeridiansIDs = {};
    std::vector<uint64_t> meshIDs = {};
    std::vector<uint64_t> tubesIDs = {};
    std::vector<uint64_t> sticksIDs = {};
    std::shared_ptr<Planet> planet;

    std::cout << "app::run()" << std::endl;
    auto trackballCamera = static_cast<TrackballCamera*>(_camera.get());

    SDL_Event event;
    auto running = true;
    auto lastTime = SDL_GetTicks();
    Uint32 currentTime = 0;
    float deltaTime = 0.0f;

    // GA
    std::future<std::shared_ptr<PlanetGA>> futureGA;
    std::shared_ptr<PlanetGA> ga = nullptr;
    
    bool gaShouldLoop = false;
    int currentPlanet = 0;
    
    bool looping = false;
    bool dirtyPlanets = true;
    bool gaLaunched = false;
    bool initializing = false;

    // MESH
    std::shared_ptr<Mesh> currentMesh;

    // Debug Ball mesh
    auto ballMesh = meshLoader->loadMesh(Apple::resourcePath("ico.fbx"));
    auto ballMeshID = _renderer->addRenderable(
        *ballMesh,
        Rendering::psoConfigs.at("VCPHONG"),
        {},
        Rendering::RenderLayer::OPAQUE
        );

    // draw options
    float tessellationResolution = 0.03f;
    bool showControlPolyhedron = false;
    bool showMesh = true;
    glm::vec4 meshColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
    bool normals = false;
    glm::vec4 normalColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    float normalLength = 0.3f;
    float normalStep = 0.03f;
    bool curvatureBasedColoring = false;
    bool fitnessBasedColoring = false;
    bool fitnessBasedColoringDiscrete = false;
    float fitnessBasedColoringDiscreteTreshold = 0.9f;
    bool wireframe = false;
    glm::vec4 wireframeColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);

    bool shaded = true;

    while (running)
    {
        // DELTA TIME
        currentTime = SDL_GetTicks();
        deltaTime = static_cast<float>(currentTime - lastTime) / 1000.0f; // in secondi
        lastTime = currentTime;

        // INPUT LOOP
        while (SDL_PollEvent(&event))
        {
            //if (ui->processInput(event)) continue;
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (ImGui::GetIO().WantCaptureMouse) continue;

            if (event.type == SDL_QUIT)
            {
                running = false;
            }
            else if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)))
            {
                trackballCamera->pan(-event.motion.xrel * 20.0f * deltaTime);
                trackballCamera->tilt(-event.motion.yrel * 20.0f * deltaTime);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    gaShouldLoop = !gaShouldLoop;
                }
                if (event.key.keysym.sym == SDLK_s)
                {
                    //if (ga) ga->population[currentPlanet]->laplacianSmoothing(0.1f, 0.7f);
                    if (ga) ga->population[currentPlanet]->curvatureBasedSmoothing();
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    if (ga)
                    {
                        if (currentPlanet > 0)
                        {
                            currentPlanet--;
                            dirtyPlanets = true;
                        }
                    }
                }
                else if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    if (ga) {
                        if (currentPlanet < ga->population.size() - 1)
                        {
                            currentPlanet++;
                            dirtyPlanets = true;
                        }
                    }
                }
            }
        }
        // update the camera
        SDL_PumpEvents();
        int* length = nullptr;
        auto keyboardState = SDL_GetKeyboardState(length);
        if (keyboardState[SDL_SCANCODE_UP]) trackballCamera->zoom(-12.0f * deltaTime);
        if (keyboardState[SDL_SCANCODE_DOWN]) trackballCamera->zoom(12.0f * deltaTime);

        _renderer->setDebugUICallback(
            [&parallelsAndMeridiansIDs,
                &meshIDs,
                &tubesIDs,
                &planet,
                this,
                controlPointMesh,
                &sticksIDs,
                &ga,
                &currentPlanet,
                &tessellationResolution,
                &futureGA,
                &gaShouldLoop,
                &gaLaunched,
                &looping,
                &dirtyPlanets,
                &showControlPolyhedron,
                &showMesh,
                &curvatureBasedColoring,
                &normals,
                &wireframe,
                &meshColor,
                &wireframeColor,
                &normalColor,
                &normalLength,
                &normalStep,
                &shaded,
                currentMesh,
                ballMeshID,
                &fitnessBasedColoring,
                &fitnessBasedColoringDiscrete,
                &fitnessBasedColoringDiscreteTreshold,
                &initializing
                ]()
            {
                // STATIC VARIABLES (HANDLED BY UI)
                static int nParallelsCP = 14;
                static int nMeridiansCP = 14;
                static float radius = 2.0f;
                static float autointersectionStep = 0.03f;
                static int nParallelsDrawn = 50;
                static int nMeridiansDrawn = 50;
                static bool showParallelsAndMeridians = false;
                static bool showTubes = false;
                static int populationSize = 1;
                static int initMutations = 20;
                static int mutationAttempts = 100;
                static int crossoverAttempts = 100;
                static int crossoverFallbackAttempts = 100;
                static float mutationScale = 0.3f;
                static float mutationMinDistance = 0.3f;
                static float mutationMaxDistance = 1.3f;
                static int immigrationSize = 0;
                static bool immigrationReplaceWithMutatedSphere = false;
                static int nImmigrationMutation = 20;
                static int gravityComputationSampleSize = 32;
                static int gravityComputationTubesResolution = 32;
                static float diversityCoefficient = 0.4f;
                static auto debugBallScale = 0.1f;


                ImGui::Begin("Drawing Options");
                ImGui::PushItemWidth(150);
                if (ImGui::Checkbox("Show Control Polyhedron", &showControlPolyhedron)) dirtyPlanets = true;
                if (ImGui::Checkbox("Show Mesh", &showMesh)) dirtyPlanets = true;
                if (ImGui::ColorEdit4("mesh color", &meshColor[0])) dirtyPlanets = true;
                if (ImGui::SliderFloat("tessellation step", &tessellationResolution, 0.01f, 0.1f, "%.2f")) dirtyPlanets = true;
                if (ImGui::Checkbox("Wireframe", &wireframe)) dirtyPlanets = true;
                if (ImGui::ColorEdit4("wireframe color", &wireframeColor[0])) dirtyPlanets = true;
                if (ImGui::Checkbox("Show Normals", &normals)) dirtyPlanets = true;
                if (ImGui::ColorEdit4("normal color", &normalColor[0])) dirtyPlanets = true;
                if (ImGui::SliderFloat("normal length", &normalLength, 0.01f, 1.0f, "%.2f")) dirtyPlanets = true;
                if (ImGui::SliderFloat("normal step", &normalStep, 0.01f, 0.1f, "%.2f")) dirtyPlanets = true;
                if (ImGui::Checkbox("Curvature Based Coloring", &curvatureBasedColoring))
                {
                    if (curvatureBasedColoring) fitnessBasedColoring = false;
                    dirtyPlanets = true;
                }
                if (ImGui::Checkbox("Fitness Based Coloring", &fitnessBasedColoring))
                {
                    if (fitnessBasedColoring) curvatureBasedColoring = false;
                    dirtyPlanets = true;
                }
                if (ImGui::Checkbox("Fitness Based Coloring Discrete", &fitnessBasedColoringDiscrete)) dirtyPlanets = true;
                if (ImGui::SliderFloat("Fitness Based Coloring Discrete Treshold", &fitnessBasedColoringDiscreteTreshold, 0.0f, 1.0f, "%.2f")) dirtyPlanets = true;

                if (ImGui::Checkbox("Shaded", &shaded)) dirtyPlanets = true;
                ImGui::SliderFloat("Debug Ball Scale", &debugBallScale, 0.01f, 1.0f, "%.2f");

                if (currentMesh && ga)
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    //ImGui::Text("mouse position: %d, %d", mouseX, mouseY);
                    int screenWidth, screenHeight;
                    SDL_GetWindowSize(_window, &screenWidth, &screenHeight);
                    //ImGui::Text("screen size: %d x %d", screenWidth, screenHeight);

                    auto mouseXNDC = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(screenWidth) - 1.0f;
                    auto mouseYNDC = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(screenHeight); // Y is inverted in window coordinates
                    //ImGui::Text("mouse position in normalized coordinates: %f, %f", mouseXNDC, mouseYNDC);

                    glm::mat4 invVP = glm::inverse(_renderer->getProjectionMatrix() * _camera->getViewMatrix());

                    auto mouseNear = glm::vec4(mouseXNDC, mouseYNDC, 0.0f, 1.0f);
                    auto mouseFar = glm::vec4(mouseXNDC, mouseYNDC, 1.0f, 1.0f);
                    mouseNear = invVP * mouseNear;
                    mouseFar = invVP * mouseFar;
                    auto mouseNear3 = glm::vec3(mouseNear) / mouseNear.w;
                    //ImGui::Text("mouse position in world coordinates: %f, %f, %f", mouseNear3.x, mouseNear3.y, mouseNear3.z);
                    auto mouseFar3 = glm::vec3(mouseFar) / mouseFar.w;
                    //ImGui::Text("mouse position in world coordinates: %f, %f, %f", mouseFar3.x, mouseFar3.y, mouseFar3.z);
                    auto dir = glm::normalize(mouseFar3 - mouseNear3);
                    //ImGui::Text("mouse direction: %f, %f, %f", dir.x, dir.y, dir.z);
                    auto uv = currentMesh->uvFromRay(mouseNear3, dir);
                    ImGui::Text("u: %f, v: %f", uv.x, uv.y);
                    auto intersection = currentMesh->rayIntersection(mouseNear3, dir);
                    ImGui::Text("Gauss Curvature: %f", ga->population[currentPlanet]->gaussCurvature(uv.x, uv.y));
                    ImGui::Text("First Fundamental Form: %f", ga->population[currentPlanet]->firstFundamentalForm(uv.x, uv.y));
                    ImGui::Text("Second Fundamental Form: %f", ga->population[currentPlanet]->secondFundamentalForm(uv.x, uv.y));
                    auto Su = ga->population[currentPlanet]->uSecondDerivative(uv.x, uv.y);
                    auto Sv = ga->population[currentPlanet]->vSecondDerivative(uv.x, uv.y);
                    ImGui::Text("Su: %f, Sv: %f, Sv dot Su: %f", glm::length(Su), glm::length(Sv), glm::dot(Su, Sv));
                    // mode debug ball
                    auto ballModelMatrix = glm::scale(glm::translate(glm::mat4(1.0f), intersection), glm::vec3(debugBallScale));
                    _renderer->modelMatrix(ballMeshID, ballModelMatrix);
                }

                ImGui::End();


                // UI -> PARAMETERS
                ImGui::Begin("Planets Generation and Evolution");
                ImGui::PushItemWidth(80);
                ImGui::InputInt("n parallels", &nParallelsCP);
                ImGui::InputInt("n meridians", &nMeridiansCP);
                ImGui::InputFloat("radius", &radius);
                ImGui::InputFloat("auto intersection step", &autointersectionStep);
                ImGui::InputInt("population size", &populationSize);
                ImGui::Separator();
                ImGui::Text("Immigration parameters:");
                ImGui::InputInt("immigration size", &immigrationSize);
                ImGui::Checkbox("immigration replace with mutated sphere", &immigrationReplaceWithMutatedSphere);
                ImGui::InputInt("n immigrant mutations", &nImmigrationMutation);
                ImGui::Separator();
                ImGui::Text("Init mutation parameters:");
                ImGui::InputInt("initial mutations", &initMutations);
                ImGui::InputFloat("min distance", &mutationMinDistance);
                ImGui::InputFloat("max distance", &mutationMaxDistance);
                ImGui::Separator();
                ImGui::Text("Differential mutation parameters:");
                ImGui::InputInt("mutation attempts", &mutationAttempts);
                ImGui::InputFloat("mutation scale", &mutationScale);
                ImGui::Separator();
                ImGui::Text("crossover parameters:");
                ImGui::InputInt("crossover attempts", &crossoverAttempts);
                ImGui::InputInt("crossover fallback attempts", &crossoverFallbackAttempts);
                ImGui::Text("Fitness computation parameters:");
                ImGui::InputInt("gravity sample res", &gravityComputationSampleSize);
                ImGui::InputInt("gravity tubes res", &gravityComputationTubesResolution);
                ImGui::InputFloat("diversity coefficient", &diversityCoefficient);
                
                //ImGui::InputInt("nParallelsDrawn", &nParallelsDrawn);
                //ImGui::InputInt("nMeridiansDrawn", &nMeridiansDrawn);
                
                ImGui::Text("current planet index: %d", currentPlanet);
                ImGui::Text("current planet fitness: %f", ga ? ga->population[currentPlanet]->fitness() : 0.0f);
                
                if (ImGui::Button("Init Evolutionary Algorithm"))
                {
                    initializing = true;
                    if (ga) ga = nullptr;
                    // INIT EVOLUTIONARY ALGORITHM
                    futureGA = std::async(std::launch::async, [&]() {
                        auto gas = std::make_shared<PlanetGA>(
                                                              populationSize,
                                                              nParallelsCP,
                                                              nMeridiansCP,
                                                              radius,
                                                              initMutations,
                                                              true,
                                                              immigrationSize,
                                                              mutationScale,
                                                              mutationMinDistance,
                                                              mutationMaxDistance,
                                                              mutationAttempts
                                                              );
                        gas
                        ->crossoverType(Uniform)
                            .crossoverAttempts(crossoverAttempts)
                            .crossoverFallbackAttempts(crossoverFallbackAttempts)
                            .crossoverFallbackToContinuous(true)
                            .diversityCoefficient(diversityCoefficient)
                            .gravityComputationSampleSize(gravityComputationSampleSize)
                            .gravityComputationTubesResolution(gravityComputationTubesResolution)
                            .autointersectionStep(autointersectionStep)
                            .immigrationReplaceWithMutatedSphere(immigrationReplaceWithMutatedSphere)
                            .nImmigrationMutations(nImmigrationMutation);

                        dirtyPlanets = true;
                        initializing = false;
                        return gas;
                    });
                }

                if (gaShouldLoop)
                    gaLaunched = true;

                if (initializing)
                {
                    ImGui::Text("Initializing... %f");
                }
                if (gaLaunched)
                {
                    ImGui::Text("Looping: %s", looping ? "true" : "false");
                    ImGui::Text("Epoch: %d", ga ? ga->epoch : 0);
                    ImGui::Text("Mean Fitness: %.3f", ga ? ga->lastMeanFitness : 0.0f);
                }
                
                ImGui::End();
            });

        // REAL TIME GA UPDATE
        // check if ga is ready and consume it
        if (!ga && futureGA.valid() && futureGA.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
            ga = futureGA.get();
        }
        
        if (ga and dirtyPlanets)// and !looping)
        {
            for (auto& id: meshIDs)
            {
                _renderer->removeRenderable(id);
            }
            meshIDs.clear();
            if (showMesh)
            {
                std::uint64_t ids;

                if (!wireframe)
                {
                    auto pso = shaded ? "VCPHONG_W" : "PC";
                    if (curvatureBasedColoring)
                    {
                        currentMesh = Mesh::fromPlanetGaussCurvatureColor(*ga->population[currentPlanet], glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                    else if (fitnessBasedColoring)
                    {
                        currentMesh = Mesh::fromPlanetFitnessColor(
                            *ga->population[currentPlanet],
                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f),
                            glm::vec4(1.0f, 1.0f, 1.0f, 1.0f),
                            tessellationResolution,
                            fitnessBasedColoringDiscrete,
                            fitnessBasedColoringDiscreteTreshold);
                    }
                    else
                    {
                        currentMesh = Mesh::fromPlanet(*ga->population[currentPlanet], meshColor, tessellationResolution);
                    }
                    ids = _renderer->addRenderable(
                                              *currentMesh,
                                              Rendering::psoConfigs.at(pso),
                                              {},
                                              Rendering::RenderLayer::OPAQUE);
                    _renderer->setWireframe(ids, wireframe);
                }
                else
                {
                    currentMesh = Mesh::fromPlanet(*ga->population[currentPlanet], wireframeColor, tessellationResolution);
                    ids = _renderer->addRenderable(
                                              *currentMesh,
                                              Rendering::psoConfigs.at("PC"),
                                              {},
                                              Rendering::RenderLayer::OPAQUE);
                    _renderer->setWireframe(ids, wireframe);
                }

                meshIDs.push_back(ids);
            }

            // draw normal sticks
            if (normals)
            {
                auto normalSticks = ga->population[currentPlanet]->normalSticks(normalLength, normalStep);
                auto sticksMesh = Mesh::fromPolygon(normalSticks, normalColor, false);
                auto ids = _renderer->addRenderable(
                                                         *sticksMesh,
                                                         Rendering::psoConfigs.at("curve"), {},
                                                         Rendering::RenderLayer::OPAQUE
                                                         );
                meshIDs.push_back(ids);
            }
            // draw control polyhedron
            if (showControlPolyhedron)
            {
                for (const auto & i : ga->population[currentPlanet]->parallelsCP())
                {
                    auto meshCP = Mesh::fromPolygon(i, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), true);
                    meshIDs.push_back(_renderer->addRenderable(
                                                         *meshCP,
                                                         Rendering::psoConfigs.at("curve"), {},
                                                         Rendering::RenderLayer::OPAQUE
                                                         ));
                }
                // draw meridians
                for (int i = 0; i < ga->population[currentPlanet]->meridiansCP().size(); i++)
                {
                    auto meshCP = Mesh::fromPolygon(ga->population[currentPlanet]->meridiansCP()[i], glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), true);
                    meshIDs.push_back(_renderer->addRenderable(
                                                         *meshCP,
                                                         Rendering::psoConfigs.at("curve"), {},
                                                         Rendering::RenderLayer::OPAQUE
                                                         ));
                }
            }
            dirtyPlanets = false;
        }
        if (ga and gaShouldLoop) {
            if (gaShouldLoop) {
                //std::cout << "Running GA loop..." << std::endl;
                // launch async loop
                static auto voidFuture = std::async(std::launch::async, [&ga, &looping, &dirtyPlanets]() {
                    looping = true;
                    ga->loop();
                    looping = false;
                    dirtyPlanets = true;
                });
                if (voidFuture.valid() && voidFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    voidFuture = std::async(std::launch::async, [&ga, &looping]() {
                        looping = true;
                        ga->loop();
                        looping = false;
                    });
                }
            }
        }


        // rendering update
        _renderer->update(_camera->getViewMatrix());
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from app::run()" << std::endl;
    return *this;
}
