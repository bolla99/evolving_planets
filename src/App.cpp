//
// Created by Giovanni Bollati on 06/03/25.
//

#include "App.hpp"

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

    _renderer->setAmbientGlobalLight({0.2f, 0.0f, 0.0f, 1.0f});

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
    std::shared_ptr<Planet> planet;

    std::cout << "app::run()" << std::endl;
    auto trackballCamera = static_cast<TrackballCamera*>(_camera.get());

    SDL_Event event;
    auto running = true;
    auto lastTime = SDL_GetTicks();
    Uint32 currentTime = 0;
    float deltaTime = 0.0f;

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
            }
        }
        // update the camera
        SDL_PumpEvents();
        int* length = nullptr;
        auto keyboardState = SDL_GetKeyboardState(length);
        if (keyboardState[SDL_SCANCODE_UP]) trackballCamera->zoom(-12.0f * deltaTime);
        if (keyboardState[SDL_SCANCODE_DOWN]) trackballCamera->zoom(12.0f * deltaTime);

        _renderer->setDebugUICallback(
            [&parallelsAndMeridiansIDs, &meshIDs, &tubesIDs, &planet, this, controlPointMesh]()
            {
                static int nParallelsCP = 14;
                static int nMeridiansCP = 14;
                static float step = 0.01f;
                static int nParallelsDrawn = 50;
                static int nMeridiansDrawn = 50;
                static float tesselationResolution = 0.01f;
                static bool showParallelsAndMeridians = false;
                static bool showMesh = false;
                static bool showTubes = false;
                static bool wireframe = false;

                ImGui::Begin("Asteroids");
                ImGui::InputInt("nParallelsCP", &nParallelsCP);
                ImGui::InputInt("nMeridiansCP", &nMeridiansCP);
                ImGui::InputFloat("step", &step);
                if (ImGui::SliderFloat("tessellation resolution", &tesselationResolution, 0.01f, 0.1f))
                {
                    if (planet)
                    {
                        for (auto& id: meshIDs)
                        {
                            _renderer->removeRenderable(id);
                        }
                        meshIDs.clear();
                        auto mesh = Mesh::fromPlanet(*planet, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), tesselationResolution);
                        meshIDs.push_back(
                            _renderer->addRenderable(
                                *mesh, Rendering::psoConfigs.at("VCPHONG_W"), {}, Rendering::RenderLayer::OPAQUE
                                )
                            );
                        _renderer->setWireframe(meshIDs.back(), wireframe);
                        _renderer->setVisible(meshIDs.back(), showMesh);
                    }
                };
                ImGui::InputInt("nParallelsDrawn", &nParallelsDrawn);
                ImGui::InputInt("nMeridiansDrawn", &nMeridiansDrawn);

                if (ImGui::Checkbox("Show parallels and meridians", &showParallelsAndMeridians))
                {
                    if (planet)
                    {
                        for (auto& id: parallelsAndMeridiansIDs)
                        {
                            _renderer->setVisible(id, showParallelsAndMeridians);
                        }
                    }
                }
                if (ImGui::Checkbox("Show mesh", &showMesh))
                {
                    if (planet)
                    {
                        for (auto& id: meshIDs)
                        {
                            _renderer->setVisible(id, showMesh);
                        }
                    }
                }
                if (ImGui::Checkbox("Show tubes", &showTubes))
                {
                    if (planet)
                    {
                        for (auto& id: tubesIDs) _renderer->setVisible(id, showTubes);
                    }
                }
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

                if (ImGui::Button("Regenerate Planet"))
                {
                    for (auto& id: parallelsAndMeridiansIDs)
                    {
                        _renderer->removeRenderable(id);
                    }
                    parallelsAndMeridiansIDs.clear();
                    for (auto& id: meshIDs)
                    {
                        _renderer->removeRenderable(id);
                    }
                    meshIDs.clear();
                    for (auto& id: tubesIDs)
                    {
                        _renderer->removeRenderable(id);
                    }
                    tubesIDs.clear();

                    //auto planet = Planet::sphere(nParallelsCP, nMeridiansCP, 2.0f);
                    planet = Planet::asteroid(nParallelsCP, nMeridiansCP, 2.0f);

                    auto parallels = planet->trueParallels(step, nParallelsDrawn);
                    for (auto& parallel: parallels)
                    {
                        auto mesh = Mesh::fromPolygon(
                        parallel,
                        glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                        auto id = _renderer->addRenderable(*mesh, Rendering::psoConfigs.at("curve"), {});
                        _renderer->setVisible(id, showParallelsAndMeridians);
                        parallelsAndMeridiansIDs.push_back(id);
                    }

                    auto meridians = planet->trueMeridians(step, nMeridiansDrawn);
                    for (auto& meridian: meridians)
                    {
                        auto mesh = Mesh::fromPolygon(
                            meridian,
                            glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
                        auto id = _renderer->addRenderable(*mesh, Rendering::psoConfigs.at("curve"), {});
                        parallelsAndMeridiansIDs.push_back(id);
                        _renderer->setVisible(id, showParallelsAndMeridians);
                    }

                    auto mesh = Mesh::fromPlanet(*planet, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), tesselationResolution);
                    auto id = _renderer->addRenderable(
                            *mesh, Rendering::psoConfigs.at("VCPHONG_W"), {}, Rendering::RenderLayer::OPAQUE
                            );
                    meshIDs.push_back(id);
                    _renderer->setVisible(id, showMesh);

                    /*
                    auto cps = planet->controlPoints();
                    for (const auto& cp : cps)
                    {
                        auto id = _renderer->addRenderable(
                            *controlPointMesh,
                            Rendering::psoConfigs.at("VCPHONG"),
                            {},
                            Rendering::RenderLayer::OPAQUE
                        );
                        meshIDs.push_back(id);
                        _renderer->modelMatrix(id, glm::translate(_renderer->modelMatrix(id), cp));
                        _renderer->modelMatrix(id, glm::scale(_renderer->modelMatrix(id), glm::vec3(0.1f)));
                    }
                    */

                    // tubes
                    auto cp = GravityAdapter::GravityComputer(*mesh);
                    auto tubes = cp.getTubes();
                    auto t = Timer();
                    for (int i = 0; i < 1000; i++)
                    {
                        auto x = cp.getGravityCPU(glm::vec3(1.0f, 1.0f, 1.0f));
                    }
                    t.log();
                    auto t2 = Timer();
                    for (int i = 0; i < 1000; i++)
                    {
                        auto x = cp.getGravityGPU(glm::vec3(1.0f, 1.0f, 1.0f));
                    }
                    t2.log();
                    auto tubesMesh = Mesh::fromPolygon(
                        tubes, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), false);
                    auto _tIDs = _renderer->addRenderable(
                            *tubesMesh, Rendering::psoConfigs.at("curve"), {}
                            );
                    _renderer->setVisible(_tIDs, showTubes);
                    tubesIDs.push_back(_tIDs);

                }
                if (ImGui::Button("Update mesh"))
                {
                    for (auto& id: meshIDs)
                    {
                        _renderer->removeRenderable(id);
                    }
                    meshIDs.clear();
                    auto mesh = Mesh::fromPlanet(*planet, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), tesselationResolution);
                    meshIDs.push_back(
                        _renderer->addRenderable(
                            *mesh, Rendering::psoConfigs.at("VCPHONG"), {}, Rendering::RenderLayer::OPAQUE
                            )
                        );
                }
                ImGui::End();
            });


        // rendering update
        _renderer->update(_camera->getViewMatrix());
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from app::run()" << std::endl;
    return *this;
}