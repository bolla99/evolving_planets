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
#include "timer.hpp"

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
    float tessellationResolution = 0.03f;
    
    bool looping = false;
    bool dirtyPlanets = true;
    bool gaLaunched = false;

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
                    if (ga) ga->population[currentPlanet]->laplacianSmoothing(0.1f, 0.7f);
                }
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    if (ga)
                    {
                        if (currentPlanet > 0)
                            currentPlanet--;
                    }
                }
                else if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    if (ga) {
                        if (currentPlanet < ga->population.size() - 1)
                            currentPlanet++;
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
                                      [&parallelsAndMeridiansIDs, &meshIDs, &tubesIDs, &planet, this, controlPointMesh, &sticksIDs, &ga, &currentPlanet, &tessellationResolution, &futureGA, &gaShouldLoop, &gaLaunched, &looping]()
            {
                // STATIC VARIABLES (HANDLED BY UI)
                static int nParallelsCP = 14;
                static int nMeridiansCP = 14;
                static float autointersectionStep = 0.01f;
                static int nParallelsDrawn = 50;
                static int nMeridiansDrawn = 50;
                static bool showParallelsAndMeridians = false;
                static bool showMesh = true;
                static bool showTubes = false;
                static bool wireframe = false;
                static bool normals = false;
                static int populationSize = 20;
                static int initMutations = 15;
                static int mutationAttempts = 15;
                static int crossoverAttempts = 20;
                static float mutationScale = 0.3f;
                static float mutationMinDistance = 0.3f;
                static float mutationMaxDistance = 1.3f;
                static int immigrationSize = 0;
                static int gravityComputationSampleSize = 32;
                static int gravityComputationTubesResolution = 32;
                static float diversityCoefficient = 0.0f;
                
                // UI -> PARAMETERS
                ImGui::Begin("Planets Generation and Evolution");
                ImGui::PushItemWidth(80);

                ImGui::InputInt("n parallels", &nParallelsCP);
                ImGui::InputInt("n meridians", &nMeridiansCP);
                ImGui::InputFloat("auto intersection step", &autointersectionStep);
                if (ImGui::SliderFloat("tessellation resolution", &tessellationResolution, 0.01f, 0.03f, "%.2f"));
                ImGui::InputInt("population size", &populationSize);
                ImGui::InputInt("immigration size", &immigrationSize);
                ImGui::Separator();
                ImGui::Text("Mutation parameters:");
                ImGui::InputInt("initial mutations", &initMutations);
                ImGui::InputInt("mutation attempts", &mutationAttempts);
                ImGui::InputFloat("mutation scale", &mutationScale);
                ImGui::InputFloat("min distance", &mutationMinDistance);
                ImGui::InputFloat("max distance", &mutationMaxDistance);
                ImGui::Separator();
                ImGui::Text("crossover parameters:");
                ImGui::InputInt("crossover attempts", &crossoverAttempts);
                ImGui::Text("Fitness computation parameters:");
                ImGui::InputInt("gravity sample res", &gravityComputationSampleSize);
                ImGui::InputInt("gravity tubes res", &gravityComputationTubesResolution);
                ImGui::InputFloat("diversity coefficient", &diversityCoefficient);
                
                //ImGui::InputInt("nParallelsDrawn", &nParallelsDrawn);
                //ImGui::InputInt("nMeridiansDrawn", &nMeridiansDrawn);

                if (ImGui::Checkbox("Wireframe", &wireframe))
                {
                    if (planet)
                    {
                        for (auto& id: meshIDs)
                        {
                            _renderer->setWireframe(id, wireframe);
                        }
                    }
                }
                
                ImGui::Text("current planet index: %d", currentPlanet);
                
                if (ImGui::Button("Init Evolutionary Algorithm"))
                {
                    // INIT EVOLUTIONARY ALGORITHM
                    futureGA = std::async(std::launch::async, [&]() {
                        auto gas = std::make_shared<PlanetGA>(
                                                              populationSize,
                                                              nParallelsCP,
                                                              nMeridiansCP,
                                                              2.0f,
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
                            .crossoverFallbackToContinuous(true)
                            .crossoverFallbackAttempts(crossoverAttempts)
                            .diversityCoefficient(diversityCoefficient)
                            .gravityComputationSampleSize(gravityComputationSampleSize)
                            .gravityComputationTubesResolution(gravityComputationTubesResolution)
                            .autointersectionStep(autointersectionStep);
                        return gas;
                    });
                }

                if (gaShouldLoop)
                    gaLaunched = true;
    
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
        
        if (ga and !looping)// and dirtyPlanets)
        {
            for (auto& id: meshIDs)
            {
                _renderer->removeRenderable(id);
            }
            meshIDs.clear();
            auto mesh = Mesh::fromPlanet(*ga->population[currentPlanet], glm::vec4(0.1f, 0.1f, 0.1f, 1.0f), tessellationResolution);
            meshIDs.push_back(_renderer->addRenderable(
                                          *mesh, Rendering::psoConfigs.at("VCPHONG_W"), {}, Rendering::RenderLayer::OPAQUE));
            dirtyPlanets = false;
        }
        if (ga and gaShouldLoop) {
            if (gaShouldLoop) {
                //std::cout << "Running GA loop..." << std::endl;
                // launch async loop
                static auto voidFuture = std::async(std::launch::async, [&ga, &looping]() {
                    looping = true;
                    ga->loop();
                    looping = false;
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
