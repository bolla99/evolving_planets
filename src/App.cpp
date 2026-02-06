
//
// Created by Giovanni Bollati on 06/03/25.
//

#include "App.hpp"
#include "PreCompileSettings.hpp"

#include <thread>
#include <atomic>
#include <future>

#include <iostream>
#include <fstream>
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
#include <imfilebrowser.h>
#include <implot.h>

#include "BSpline.hpp"
#include "imgui_impl_sdl2.h"
#include "imgui_impl_metal.h"
#include "Planet.hpp"
#include <PlanetsPopulation.hpp>
#include <gravity.hpp>
#include <GravityAdapter.hpp>
#include <ctime>
#include <cstdlib>
#include <fstream>

#include "genetic.hpp"
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/types/vector.hpp>

#include "BVH.hpp"

// app constructor initialize sdl, create a window and try to
// create a renderer from the custom class Renderer, which is in turn responsible
// for the acquisition of the sdl renderer, besides the metal structures.
App::App(
    const int width,
    const int height
    ) //: _camera(std::move(std::make_unique<TrackballCamera>()))
{
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImPlot::CreateContext();

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

std::array<glm::vec3, 2> App::mouseRay(glm::mat4 viewMatrix, SDL_Window* window, Rendering::IRenderer* renderer)
{
    // set mouse ray casting
    int mouseX, mouseY;
    SDL_GetMouseState(&mouseX, &mouseY);
    int screenWidth, screenHeight;
    SDL_GetWindowSize(window, &screenWidth, &screenHeight);

    auto mouseXNDC = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(screenWidth) - 1.0f;
    auto mouseYNDC = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(screenHeight); // Y is inverted in window coordinates
    glm::mat4 invVP = glm::inverse(renderer->getProjectionMatrix() * viewMatrix);
    auto mouseNear = glm::vec4(mouseXNDC, mouseYNDC, 0.0f, 1.0f);
    auto mouseFar = glm::vec4(mouseXNDC, mouseYNDC, 1.0f, 1.0f);
    mouseNear = invVP * mouseNear;
    mouseFar = invVP * mouseFar;
    auto mouseNear3 = glm::vec3(mouseNear) / mouseNear.w;
    auto mouseFar3 = glm::vec3(mouseFar) / mouseFar.w;
    auto dir = glm::normalize(mouseFar3 - mouseNear3);
    return {mouseNear3, dir};
}
