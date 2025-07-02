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

#include <glm/glm.hpp>
#include <Apple/Util.hpp>
#include "TrackballCamera.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <Rendering/UI/UIManager.hpp>
#include <Rendering/UI/UIRenderer.hpp>


// app constructor initialize sdl, create a window and try to
// create a renderer from the custom class Renderer, which is in turn responsible
// for the acquisition of the sdl renderer, besides the metal structures.
App::App(
    const int width,
    const int height
    ) : _camera(std::move(std::make_unique<TrackballCamera>()))
{
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
            //auto text = Texture::fromText("cane peloso", 30, {0.5f, 0.5f, 0.5f, 1.0f});
            _renderer->addRenderable(
                *mesh,
                Rendering::psoConfigs.at("TexturePHONG"), {texture});

            float pos[3] = {100.0f, 100.0f, 0.0f};
            glm::vec4 color = {1.0f, 0.0f, 0.0f, 1.0f};
            /*auto textBox = Mesh::quad(
                pos,
                text->width(), text->height(),
                glm::value_ptr(color), 1.0f);
            _renderer->addRenderable(
                *textBox,
                Rendering::psoConfigs.at("TextureUI"), {text}
                );*/
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
    std::cout << "app::run()" << std::endl;
    auto trackballCamera = static_cast<TrackballCamera*>(_camera.get());

    auto ui = std::make_unique<Rendering::UI::UIManager>(
        std::make_unique<Rendering::UI::UIRenderer>(
            _renderer.get(),
            Rendering::psoConfigs.at("UI"),
            Rendering::UI::UIWindowStyle::defaultStyle())
        );

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

        while (SDL_PollEvent(&event))
        {
            if (ui->processInput(event)) continue;
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
        int* length = nullptr;
        auto keyboardState = SDL_GetKeyboardState(length);
        if (keyboardState[SDL_SCANCODE_UP])
        {
            trackballCamera->zoom(-12.0f * deltaTime);
        }
        if (keyboardState[SDL_SCANCODE_DOWN])
        {
            trackballCamera->zoom(12.0f * deltaTime);
        }

        ui->clear();

        ui->beginWindow("Cane");
        ui->text("Ciao, sono un cane peloso");
        ui->text("triceratopo");
        ui->endWindow();

        ui->beginWindow("Cane maremmano");
        ui->text("Ciao, sono un cane maremmano");
        ui->endWindow();

        ui->update();
        _renderer->update(_camera->getViewMatrix());
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from app::run()" << std::endl;
    return *this;
}
