//
// Created by Giovanni Bollati on 29/01/26.
//


//#if COMPILE_EVOLVING_PLANETS_APP == 1

#include "EvolvingPlanetsApp.hpp"

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

template <typename T>
concept IsFuture = requires(T f) {
    // Richiediamo che T sia una specializzazione di std::future
    requires std::is_same_v<T, std::future<typename T::value_type>>;
};

bool isFutureRunning(IsFuture auto& f) {
    return f.valid() and f.wait_for(std::chrono::seconds(0)) != std::future_status::ready;
}

void EvolvingPlanetsApp::init()
{
    // set aspect ratio
    //_renderer->setAspect({1.0f/3.0f, 0.0f, 2.0f/3.0f, 3.0f/4.0f});

    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    std::cout << "app::init()" << std::endl;

    // load a Geometry::Mesh from file and add it to the renderer
    auto meshLoader = std::make_unique<AssimpMeshLoader>();



    /*
    _renderer->setDirectionalLight(
        {
            {1.0f, -1.0f, 0.0f, 0.0f},
            {0.3f, 0.3f, 0.3f, 1.0f}
        }, 0);

    _renderer->setPointLight({
            {1.0f, -1.0f, 1.0f, 1.0f},
            {0.05f, 0.05f, 0.2f, 1.0f}
        }, 1);

    _renderer->setPointLight({
            {-1.0f, 1.0f, 1.0f, 1.0f},
            {0.1f, 0.1f, 0.2f, 1.0f}
        }, 2);
    */
    //_renderer->setAmbientGlobalLight({0.2f, 0.2f, 0.2f, 1.0f});

}

// sdl loop: input management and renderer update
// right now input management is decoupled from the renderer, which means
// that the renderer is not aware of the input events, but input events can
// refer to the renderer, for example to change the state of a renderable object.
void EvolvingPlanetsApp::run()
{
    bool showPlanetDebug = false;
    static bool useConstantLOD_UI = false;
    static int constantLOD_UI = 6;
    static bool showMesh_UI = true;
    static bool isRocky_UI = false;
    static int fractalOctaves_UI = 16;
    static float fractalIntensity_UI = 5.0f;
    static float fractalScale_UI = 3.5f;
    static float normalDelta_UI = 0.0001f;
    static bool useRockyTexture_UI = false;
    static bool usePositionTexture_UI = true;
    static bool useNormalTexture_UI = true;
    static bool useSimplexNoise_UI = false;
    static bool useTriplanar_UI = true;
    glm::vec4 viewport = {1.0f/3.0f, 0.0f, 2.0f/3.0f, 3.0f/4.0f};
    // set up camera
    auto camera = TrackballCamera();
    camera.zoom(10.0f);

    auto meshLoader = std::make_unique<AssimpMeshLoader>();
    auto icoMesh = meshLoader->loadMesh(Apple::resourcePath("ico.obj"));
    auto controlPointMesh = meshLoader->loadMesh(Apple::resourcePath("ico.fbx"));

    std::vector<uint64_t> parallelsAndMeridiansIDs = {};
    std::vector<uint64_t> meshIDs = {};
    std::vector<uint64_t> tubesIDs = {};
    std::vector<uint64_t> sticksIDs = {};

    uint64_t currentMeshRenderID = 0;
    uint64_t currentMeshMaterialID = 0;
    std::vector<std::pair<uint64_t, uint64_t>> cpRenderIDs = {};

    std::shared_ptr<Planet> planet;

    std::cout << "app::run()" << std::endl;
    //auto trackballCamera = static_cast<TrackballCamera*>(_camera.get());

    SDL_Event event;
    auto running = true;

    // delta time parameters
    auto lastTime = SDL_GetTicks();
    Uint32 currentTime = 0;
    float deltaTime = 0.0f;

    // GA
    std::future<void> futureGA;
    std::future<void> voidFuture;
    std::future<void> exportMeshesFuture;
    std::future<void> exportMeshFuture;
    int currentlyExportingMesh = 0;

    std::shared_ptr<PlanetGA> ga = nullptr;

    bool gaShouldLoop = false;
    int currentPlanet = 0;

    bool looping = false;
    bool dirtyPlanets = true;

    // this flag is set to true to signal the thread running the initialization
    // that is should stop
    bool initShouldStop = false;

    // current planet Geometry::Mesh being rendered
    std::shared_ptr<Geometry::Mesh> currentMesh;// = Geometry::Mesh::icosphere(3);
    std::shared_ptr<Geometry::Mesh> meshShaderTestMesh = Geometry::Mesh::icosphere(3);

    std::vector<std::shared_ptr<Geometry::Mesh>> cpMeshes;

    // Debug Ball Geometry::Mesh
#if DEBUG_BALL
    auto ballMesh = meshLoader->loadMesh(Apple::resourcePath("ico.fbx"));
    auto ballMeshID = _renderer->addRenderable(
        *ballMesh,
        Rendering::psoConfigs.at("VCPHONG"),
        {},
        Rendering::RenderLayer::OPAQUE
        );
    // set debug ball not visible by default
    _renderer->setVisible(ballMeshID, false);
#endif

    // --- TEST Geometry::Mesh SHADER ---
    //auto testMesh = Geometry::Mesh::icosphere(3);
    auto meshPSO = Rendering::psoConfigs.at("TestMesh");
    // Create a material instance for TestMesh

    // draw options -> parameters that are needed both by the ui and the rendering update that happens outside the
    // the ui callback
    float tessellationResolution = 0.03f;
    bool showControlPolyhedron = false;
    bool showMesh = true;
    glm::vec4 meshColor = glm::vec4(0.7f, 0.7f, 0.7f, 1.0f);
    bool normals = false;
    glm::vec4 normalColor = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    float normalLength = 0.3f;
    float normalStep = 0.03f;
    bool curvatureBasedColoring = false;
    bool fitnessBasedColoring = false;
    bool fitnessBasedColoringDiscrete = false;
    float fitnessBasedColoringDiscreteTreshold = 0.9f;
    bool wireframe = false;
    glm::vec4 wireframeColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    bool shaded = true;
    bool isRocky = false;
    bool ico = false;
    bool cube = false;
    bool usePoissonDisk = false;
    int icoSubdivisions = 4;
    int cubeSubdivisions = 4;
    int rockyFractalOctaves = 16;
    float rockyFractalIntensity = 0.15f;
    float rockyFractalScale = 1.0f;
    float poissonMinDistance = 0.05f;

    float radius = 2.0f;

    std::vector<glm::vec3> x = {{0.0f, 0.0f, 0.0f}, {10000.0f, 0.0f, 0.0f}};
    std::vector<glm::vec3> y = {{0.0f, 0.0f, 0.0f}, {0.0f, 10000.0f, 0.0f}};
    std::vector<glm::vec3> z = {{0.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 10000.0f}};
    auto xMesh = Geometry::Mesh::fromPolygon(x, {1.0f, 0.0f, 0.0f, 1.0f}, false);
    auto yMesh = Geometry::Mesh::fromPolygon(y, {0.0f, 1.0f, 0.0f, 1.0f}, false);
    auto zMesh = Geometry::Mesh::fromPolygon(z, {0.0f, 0.0f, 1.0f, 1.0f}, false);

    // IMGUI DIALOG
    ImGui::FileBrowser fileDialog(ImGuiFileBrowserFlags_EnterNewFilename);
    ImGui::FileBrowser populationFileDialog(ImGuiFileBrowserFlags_EnterNewFilename);
    ImGui::FileBrowser loadPopulationFileDialog(0);
    ImGui::FileBrowser everyMeshFileDialog(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);
    ImGui::FileBrowser logFileDialog(ImGuiFileBrowserFlags_EnterNewFilename | ImGuiFileBrowserFlags_CreateNewDir);

    std::string exportPath;
    std::string populationExportPath;
    std::string loadPopulationPath;

    // (optional) set browser properties
    fileDialog.SetTitle("Export Mesh");
    populationFileDialog.SetTitle("Export Population");
    loadPopulationFileDialog.SetTitle("Load Population File");
    loadPopulationFileDialog.SetTypeFilters({".population"});

    while (running)
    {
        // UPDATE DELTA TIME
        currentTime = SDL_GetTicks();
        deltaTime = static_cast<float>(currentTime - lastTime) / 1000.0f; // in secondi
        lastTime = currentTime;

        // INPUT LOOP
        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
            {
                if (voidFuture.valid()) voidFuture.wait();
                running = false;
            }
            //if (ui->processInput(event)) continue;
            ImGui_ImplSDL2_ProcessEvent(&event);
            if (ImGui::GetIO().WantCaptureMouse) continue;

            // ROTATE CAMERA
            else if (event.type == SDL_MOUSEMOTION && (event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT)))
            {
                camera.pan(-event.motion.xrel * 20.0f * deltaTime);
                camera.tilt(-event.motion.yrel * 20.0f * deltaTime);
            }
            else if (event.type == SDL_KEYDOWN)
            {
                // CLOSE SOFTWARE
                if (event.key.keysym.sym == SDLK_ESCAPE)
                {
                    running = false;
                }
                /*
                if (event.key.keysym.sym == SDLK_s)
                {
                    //if (ga) ga->population[currentPlanet]->laplacianSmoothing(0.1f, 0.7f);
                    if (ga) ga->population[currentPlanet]->curvatureBasedSmoothing();
                }
                */

                if (event.key.keysym.sym == SDLK_SPACE)
                {
                    if (currentMesh)
                    {
                        auto path = "/Users/bellobolla/cane.obj";
                        meshLoader->saveMesh(path, currentMesh);
                    }
                }

                // LEFT ARROW KEY
                if (event.key.keysym.sym == SDLK_LEFT)
                {
                    if (ga and ga->hasInitialized())
                    {
                        if (currentPlanet > 0)
                        {
                            currentPlanet--;
                            dirtyPlanets = true;
                        }
                    }
                }
                // RIGHT ARROW KEY
                else if (event.key.keysym.sym == SDLK_RIGHT)
                {
                    if (ga and ga->hasInitialized()) {
                        if (currentPlanet < ga->population.size() - 1)
                        {
                            currentPlanet++;
                            dirtyPlanets = true;
                        }
                    }
                }
            }
        }
        // UPDATE CAMERA ZOOM
        SDL_PumpEvents();
        int* length = nullptr;
        auto keyboardState = SDL_GetKeyboardState(length);
        auto cameraSpeed = 1.0f;
        if (keyboardState[SDL_SCANCODE_SPACE]) cameraSpeed = 10.0f;
        if (keyboardState[SDL_SCANCODE_UP]) camera.zoom(-12.0f * cameraSpeed * deltaTime);
        if (keyboardState[SDL_SCANCODE_DOWN]) camera.zoom(12.0f * cameraSpeed * deltaTime);

        // UPDATE LIGHT
        // world.getComponent<DirectionalLightComponent>(directionalLight).light.direction = glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        /*_renderer->setDirectionalLight(
        {
            -glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f),
            {0.5f, 0.5f, 0.5f, 1.0f}
        }, 0);*/

        auto cameraDir = glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        //std::cout << cameraDir.x << " " << cameraDir.y << " " << cameraDir.z << std::endl;

        // set debug ui callback: this function will be executed by the renderer inside its rendering loop
        // must be reset every frame in order to update its
        if (_renderer) {
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
                &voidFuture,
                &gaShouldLoop,
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
                &radius,
#if DEBUF_BALL
                ballMeshID,
#endif
                &fitnessBasedColoring,
                &fitnessBasedColoringDiscrete,
                &fitnessBasedColoringDiscreteTreshold,
                &fileDialog,
                &populationFileDialog,
                &populationExportPath,
                &exportPath,
                &meshLoader,
                &initShouldStop,
                &loadPopulationFileDialog,
                &everyMeshFileDialog,
                &exportMeshesFuture,
                &currentlyExportingMesh,
                &exportMeshFuture,
                &logFileDialog, camera, &isRocky, &ico, &cube, &icoSubdivisions, &cubeSubdivisions, &rockyFractalOctaves, &rockyFractalIntensity, &rockyFractalScale, &usePoissonDisk, &poissonMinDistance, &showPlanetDebug
            ]()
            {
                // STATIC VARIABLES (HANDLED BY UI)
                static int nParallelsCP = 15;
                static int nMeridiansCP = 14;
                static float autointersectionStep = 0.03f;
                static int nParallelsDrawn = 50;
                static int nMeridiansDrawn = 50;
                static bool showParallelsAndMeridians = false;
                static bool showTubes = false;
                static int populationSize = 30;
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
                static int immigrationType = 0;
                static int gravityComputationSampleSize = 32;
                static int gravityComputationTubesResolution = 32;
                static float diversityCoefficient = 0.2f;
                static auto debugBallScale = 0.1f;
                static float diversityLimit = 0.1;
                static float fitnessThreshold = 0.001;
                static int  epochWithNoImprovement = 100;
                static int maxIterations = 1000;
                static bool adaptiveMutationRate = true;
                static int fitnessType = 0;
                static float distanceFromSurface = 0.0f;
                static bool initFromPopulationFile = false;
                static bool showErrorPopup = false;
                static std::string errorPopupMessage = "";
                static bool showExportMeshesPopup = false;
                static bool showExportMeshPopup = false;


                // DRAW OPTIONS
                int w, h; SDL_GetWindowSize(_window, &w, &h);
                ImGui::SetNextWindowPos({static_cast<float>(w) / 3.0f, 3.0f * static_cast<float>(h) / 4.0f});
                ImGui::SetNextWindowSize({2.0f * static_cast<float>(w) / 3.0f + 1, static_cast<float>(h) / 4.0f + 1});
                ImGui::Begin("Drawing Options", 0, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse);
                ImGui::PushItemWidth(150);
                if (ImGui::Checkbox("Show Control Polyhedron", &showControlPolyhedron)) dirtyPlanets = true;
                if (ImGui::Checkbox("Show Mesh", &showMesh)) {}//dirtyPlanets = true;
                //if (ImGui::ColorEdit4("mesh color", &meshColor[0])) dirtyPlanets = true;
                //if (ImGui::SliderFloat("tessellation step", &tessellationResolution, 0.001f, 0.05f, "%.3f")) dirtyPlanets = true;
                if (ImGui::Checkbox("Wireframe", &wireframe)) {}//dirtyPlanets = true;
                //if (ImGui::Checkbox("Is Rocky", &isRocky)) dirtyPlanets = true;
                //if (ImGui::Checkbox("Ico", &ico)) dirtyPlanets = true;
                //if (ImGui::Checkbox("Cube", &cube)) dirtyPlanets = true;
                //if (ImGui::Checkbox("Poisson Disk", &usePoissonDisk)) dirtyPlanets = true;
                //if (ImGui::SliderInt("Ico Subdivisions", &icoSubdivisions, 0, 10)) dirtyPlanets = true;
                //if (ImGui::SliderInt("Cube Subdivisions", &cubeSubdivisions, 0, 10)) dirtyPlanets = true;
                //if (ImGui::SliderInt("Rocky Octaves", &rockyFractalOctaves, 0, 20)) dirtyPlanets = true;
                //if (ImGui::SliderFloat("Rocky Intensity", &rockyFractalIntensity, 0.0f, 5.0f, "%.2f")) //dirtyPlanets = true;
                //if (ImGui::SliderFloat("Rocky Scale", &rockyFractalScale, 0.1f, 10.0f, "%.2f")) dirtyPlanets = true;
                //if (ImGui::SliderFloat("Poisson Min Distance", &poissonMinDistance, 0.01f, 0.2f, "%.3f")) dirtyPlanets = true;
                if (ImGui::Checkbox("Use Constant LOD", &useConstantLOD_UI)) {}//dirtyPlanets = true;
                if (ImGui::SliderInt("Constant LOD Value", &constantLOD_UI, 0, 12)) {}//dirtyPlanets = true;
                if (ImGui::Checkbox("Show Mesh (Shader)", &showMesh_UI)) {}//dirtyPlanets = true;
                if (ImGui::Checkbox("Is Rocky Mesh Shader", &isRocky_UI)) {}//dirtyPlanets = true;
                if (isRocky_UI) {
                    if (ImGui::Checkbox("Use Rocky Texture", &useRockyTexture_UI)) dirtyPlanets = true;
                    if (ImGui::Checkbox("Use Simplex Noise", &useSimplexNoise_UI)) dirtyPlanets = true;
                    if (ImGui::Checkbox("Use Triplanar Noise", &useTriplanar_UI)) {}//dirtyPlanets = true;
                    if (ImGui::Checkbox("Use Position Texture", &usePositionTexture_UI)) dirtyPlanets = true;
                    if (ImGui::Checkbox("Use Normal Texture", &useNormalTexture_UI)) dirtyPlanets = true;
                    if (ImGui::SliderInt("Octaves", &fractalOctaves_UI, 1, 20)) {}//dirtyPlanets = true;
                    if (ImGui::SliderFloat("Intensity", &fractalIntensity_UI, 0.0f, 10.0f)) {}//dirtyPlanets = true;
                    if (ImGui::SliderFloat("Scale", &fractalScale_UI, 0.01f, 1.0f)) {}//dirtyPlanets = true;
                    if (ImGui::SliderFloat("Normal Delta", &normalDelta_UI, 0.00001f, 0.0001f, "%.6f")) {}//dirtyPlanets = true;
                }
                //if (ImGui::Checkbox("Draw Planet Debug", &showPlanetDebug)) dirtyPlanets = true;
                //if (ImGui::ColorEdit4("wireframe color", &wireframeColor[0])) dirtyPlanets = true;
                /*
                if (ImGui::Checkbox("Show Normals", &normals)) dirtyPlanets = true;
                if (ImGui::ColorEdit4("normal color", &normalColor[0])) dirtyPlanets = true;
                if (ImGui::SliderFloat("normal length", &normalLength, 0.01f, 1.0f, "%.2f")) dirtyPlanets = true;
                if (ImGui::SliderFloat("normal step", &normalStep, 0.01f, 0.1f, "%.2f")) dirtyPlanets = true;

                if (ImGui::Checkbox("Curvature Based Coloring", &curvatureBasedColoring))
                {
                    if (curvatureBasedColoring) fitnessBasedColoring = false;
                    dirtyPlanets = true;
                }
                 */

                if (ImGui::Checkbox("Fitness Based Coloring", &fitnessBasedColoring))
                {
                    if (fitnessBasedColoring) curvatureBasedColoring = false;
                    dirtyPlanets = true;
                }

                if (ImGui::Checkbox("Fitness Based Coloring Discrete", &fitnessBasedColoringDiscrete)) dirtyPlanets = true;
                if (ImGui::SliderFloat("Fitness Based Coloring Discrete Treshold", &fitnessBasedColoringDiscreteTreshold, 0.0f, 1.0f, "%.2f")) dirtyPlanets = true;


                //if (ImGui::Checkbox("Shaded", &shaded)) dirtyPlanets = true;

#if DEBUG_BALL
                /*
                _renderer->setVisible(ballMeshID, true);
                ImGui::SliderFloat("Debug Ball Scale", &debugBallScale, 0.01f, 1.0f, "%.2f");
                if (currentMesh and ga and ga->hasInitialized())
                {
                    int mouseX, mouseY;
                    SDL_GetMouseState(&mouseX, &mouseY);
                    //ImGui::Text("mouse position: %d, %d", mouseX, mouseY);
                    int screenWidth, screenHeight;
                    SDL_GetWindowSize(_window, &screenWidth, &screenHeight);
                    //ImGui::Text("screen size: %d x %d", screenWidth, screenHeight);

                    mouseX -= screenWidth / 3.0f;
                    screenWidth *= (2.0f / 3.0f);
                    screenHeight *= (3.0f / 4.0f);
                    auto mouseXNDC = (2.0f * static_cast<float>(mouseX)) / static_cast<float>(screenWidth) - 1.0f;
                    auto mouseYNDC = 1.0f - (2.0f * static_cast<float>(mouseY)) / static_cast<float>(screenHeight); // Y is inverted in window coordinates
                    //ImGui::Text("mouse position in normalized coordinates: %f, %f", mouseXNDC, mouseYNDC);

                    glm::mat4 invVP = glm::inverse(_renderer->getProjectionMatrix() * camera.getViewMatrix());

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
                    //auto intersection = currentMesh->rayIntersection(mouseNear3, dir);
                    auto bvh = BVH(currentMesh->getVertices(), currentMesh->getFacesData());
                    glm::vec3 intersection;
                    bvh.intersect(mouseNear3, dir, currentMesh->getVertices(), currentMesh->getFacesData(), intersection);
                    ImGui::Text("intersection: %f, %f, %f", intersection.x, intersection.y, intersection.z);
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
                */
#endif

                ImGui::End();
                ImGui::SetNextWindowPos({0, 0});
                ImGui::SetNextWindowSize({static_cast<float>(w) / 3.0f, static_cast<float>(h)});
                // EVOLUTIONARY ALGORITHM PARAMETERS
                ImGui::Begin(
                    "Planets Generation and Evolution",
                    0, ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoTitleBar);
                if (ImGui::BeginTabBar("tab bar"))
                {
                    if (ImGui::BeginTabItem("EA Parameters"))
                    {
                        ImGui::BeginChild("child window");
                        ImGui::PushItemWidth(80);
                        if (ImGui::CollapsingHeader("initialization parameters")) {
                            ImGui::InputInt("n parallels", &nParallelsCP);
                            ImGui::InputInt("n meridians", &nMeridiansCP);
                            ImGui::InputFloat("radius", &radius);
                            ImGui::SliderFloat("tessellation step", &autointersectionStep, 0.01f, 0.1f, "%.2f");
                            ImGui::InputInt("initial mutations", &initMutations);
                            ImGui::InputFloat("min distance", &mutationMinDistance);
                            ImGui::InputFloat("max distance", &mutationMaxDistance);
                            ImGui::InputInt("population size", &populationSize);
                            ImGui::Checkbox("init from population file", &initFromPopulationFile);
                        }
                        if (ImGui::CollapsingHeader("immigration parameters")) {
                            std::vector<const char*> immigrationItems = {"random replacement", "replace the most similar"};
                            ImGui::PushItemWidth(220);
                            ImGui::Combo("immigration type", &immigrationType, immigrationItems.data(), 2);
                            ImGui::PushItemWidth(80);
                            ImGui::InputInt("immigration size", &immigrationSize);
                            ImGui::Checkbox("immigration replace with mutated sphere", &immigrationReplaceWithMutatedSphere);
                            ImGui::InputInt("n immigrant mutations", &nImmigrationMutation);
                            ImGui::Separator();
                        }
                        if (ImGui::CollapsingHeader("mutation parameters")) {
                            ImGui::InputInt("mutation attempts", &mutationAttempts);
                            ImGui::InputFloat("mutation scale", &mutationScale);
                        }
                        if (ImGui::CollapsingHeader("crossover parameters")) {
                            ImGui::Text("crossover parameters:");
                            ImGui::InputInt("crossover attempts", &crossoverAttempts);
                            ImGui::Checkbox("adaptive mutation rate", &adaptiveMutationRate);
                            ImGui::InputInt("crossover fallback attempts", &crossoverFallbackAttempts);
                        }
                        if (ImGui::CollapsingHeader("fitness parameters")) {

                            /*
                            std::vector<const char*> fitnessItems = {"g dot surface normal", "g dot p -> mass centre"};
                            ImGui::PushItemWidth(160);
                            ImGui::Combo("fitness model", &fitnessType, fitnessItems.data(), 2);
                             */
                            ImGui::PushItemWidth(80);
                            //ImGui::InputInt("fitness model", &fitnessType, 1);
                            /*
                            ImGui::InputFloat("distance from surface", &distanceFromSurface);*/

                            ImGui::InputInt("gravity sample res", &gravityComputationSampleSize);
                            ImGui::InputInt("gravity tubes res", &gravityComputationTubesResolution);
                            ImGui::InputFloat("diversity coefficient", &diversityCoefficient);
                        }

                        //ImGui::InputInt("nParallelsDrawn", &nParallelsDrawn);
                        //ImGui::InputInt("nMeridiansDrawn", &nMeridiansDrawn);

                        if (ImGui::CollapsingHeader("termination parameters")) {
                            ImGui::InputInt("max iterations", &maxIterations);
                            ImGui::InputFloat("diversity limit", &diversityLimit);
                            ImGui::InputFloat("fitness threshold", &fitnessThreshold);
                            ImGui::InputInt("max iterations within fitness threshold", &epochWithNoImprovement);
                        }
                        ImGui::EndChild();
                        ImGui::EndTabItem();
                    }

                    if (showErrorPopup) {
                        ImGui::OpenPopup("Error");
                    }
                    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                    if (ImGui::BeginPopupModal("Error", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                        ImGui::Text("%s", errorPopupMessage.c_str());
                        if (ImGui::Button("Close")) {
                            showErrorPopup = false;
                            ImGui::CloseCurrentPopup();
                        }
                        ImGui::EndPopup();
                    }

                    if (ImGui::BeginTabItem("Controls"))
                    {
                        // CURRENT PLANET INFO
                        if (ga and ga->hasInitialized())
                        {
                            ImGui::Text("current planet index: %d", currentPlanet);
                            ImGui::Text("current planet fitness: %f", ga->getLastFitnessValues()[currentPlanet]);
                            ImGui::Text("current planet diversity: %f", ga->getDiversity(currentPlanet));
                        }

                        if (ga == nullptr or (ga and ga->hasInitialized() and !looping)) {
                            // CREATE EVOLUTIONARY ALGORITHM (SECOND THREAD)
                            if (ImGui::Button("Init Evolutionary Algorithm"))
                            {
                                currentPlanet = 0;
                                initShouldStop = false;
                                // create the evolutionary algorithm
                                ga = std::make_shared<PlanetGA>(
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
                                ga->crossoverType(Uniform)
                                    .crossoverAttempts(crossoverAttempts)
                                    .crossoverFallbackAttempts(crossoverFallbackAttempts)
                                    .crossoverFallbackToContinuous(true)
                                    .diversityCoefficient(diversityCoefficient)
                                    .gravityComputationSampleSize(gravityComputationSampleSize)
                                    .gravityComputationTubesResolution(gravityComputationTubesResolution)
                                    .autointersectionStep(autointersectionStep)
                                    .immigrationReplaceWithMutatedSphere(immigrationReplaceWithMutatedSphere)
                                    .nImmigrationMutations(nImmigrationMutation)
                                    .diversityLimit(diversityLimit)
                                    .fitnessThreshold(fitnessThreshold)
                                    .epochWithNoImprovement(epochWithNoImprovement)
                                    .maxIterations(maxIterations)
                                    .adaptiveMutationRate(adaptiveMutationRate)
                                    .fitnessType(fitnessType)
                                    .distanceFromSurface(distanceFromSurface)
                                    .immigrationType(immigrationType);
                                if (initFromPopulationFile) {
                                    loadPopulationFileDialog.Open();
                                } else {
                                    // initialize evolutionary algorithm on second thread
                                    futureGA = std::async(std::launch::async, [&dirtyPlanets, ga, &initShouldStop]() {
                                        while(ga->initialize()) {
                                            if (initShouldStop) return;
                                        };
                                        // after initialization mark dirtyPlanets = true to trigger the Meshes update
                                        dirtyPlanets = true;
                                    });
                                }
                            }
                        }

                        loadPopulationFileDialog.Display();
                        if (loadPopulationFileDialog.IsOpened()) {
                            if (loadPopulationFileDialog.HasSelected()) {

                                try {
                                    auto p = PlanetsPopulation::load(loadPopulationFileDialog.GetSelected().string());
                                    ga->initByPopulation(p);
                                } catch(std::exception& e) {
                                    ga = nullptr;
                                    showErrorPopup = true;
                                    errorPopupMessage = e.what();
                                }
                            }
                        }

                        if (ga and !ga->hasInitialized()) {
                            if (ImGui::Button("Stop initialization")) {
                                initShouldStop = true;
                                ga = nullptr;
                            }
                        }


                        // if the evolutionary algorithm has been created and initialized, then show the button to start and pause
                        // the looping
                        if (ga and ga->hasInitialized() and !ga->shouldTerminate())
                        {
                            if (ImGui::Button("Start/Pause Evolutionary Algorithm"))
                            {
                                gaShouldLoop = !gaShouldLoop;
                            }
                        }

                        /*
                        if (ga and ga->hasInitialized())
                        {
                            if (ImGui::Button("is autointersecating"))
                            {
                                if (ga->population[currentPlanet]->isAutointersecating(autointersectionStep))
                                {
                                    std::cout << "Planet is self-intersecting" << std::endl;
                                }
                                else
                                {
                                    std::cout << "Planet is NOT self-intersecting" << std::endl;
                                }
                            }
                        }*/


                        // if the evolutionary algorithm has been created but it is still initializing, show the initialization progress
                        if (ga and !ga->hasInitialized())
                        {
                            ImGui::Text("Initializing... %d%%", static_cast<int>(100.0f * static_cast<float>(ga->currentIndividualBeingInitialized() + 1) / static_cast<float>(ga->population.size())));
                        }

                        // if the evolutionary algorithm has been created and initialized, then show info
                        if (ga and ga->hasInitialized())
                        {
                            if (ga->shouldTerminate())
                            {
                                ImGui::Text("Terminated");
                            }
                            ImGui::Text("Looping: %s", looping ? "true" : "false");
                            ImGui::Text("Epoch: %d", ga and ga->hasInitialized() ? ga->epoch : 0);
                            ImGui::Text("Mean Fitness: %.3f", ga and ga->hasInitialized() ? ga->getLastMeanFitness(): 0.0f);
                            ImGui::Text("Mean Error: %.3f", ga and ga->hasInitialized()? ga->getLastMeanError(): 0.0f);
                            ImGui::Text("Mean Diversity: %.3f", ga and ga->hasInitialized() ? ga->getMeanDiversity() : 0.0f);
                            ImGui::Text("Best: %.3f", ga->getBest());
                        }

                        if (ga and ga->hasInitialized() and currentMesh and !looping)
                        {
                            if (ImGui::Button("Export Mesh")) {
                                fileDialog.Open();
                            }

                            if (ImGui::Button("Export Every Mesh")) {
                                everyMeshFileDialog.Open();
                            }
                        }
                        if (ga and ga->hasInitialized() and !looping)
                        {
                            if (ImGui::Button("Export Population"))
                            {
                                populationFileDialog.Open();
                            }
                        }
                        if (ga and ga->hasInitialized() and !looping) {
                            if (ImGui::Button("Log Execution")) {
                                logFileDialog.Open();
                            }
                        }
                        // PLOT
                        if (ga and ga->hasInitialized()) {
                            if (ImPlot::BeginPlot("My Plot")) {
                                ImPlot::SetupAxis(ImAxis_X1, "epochs", ImPlotAxisFlags_AutoFit);

                                auto meanFitnesses = ga->getMeanFitness();
                                float epochs[ga->epoch]; for(int i = 0; i < ga->epoch; i++) epochs[i] = i;
                                auto errors = ga->getMeanErrors();
                                auto upper = std::vector<float>(errors.size());
                                auto lower = std::vector<float>(errors.size());
                                for(int i = 0; i < errors.size(); i++) {
                                    upper[i] = meanFitnesses[i] + errors[i];
                                    lower[i] = meanFitnesses[i] - errors[i];
                                }
                                ImPlot::PlotShaded("Variance", epochs, upper.data(), lower.data(), ga->epoch);
                                ImPlot::PlotLine("Mean Fitness", epochs, meanFitnesses.data(), ga->epoch);
                                ImPlot::EndPlot();
                            }
                        }
                        ImGui::EndTabItem();
                    }
                    ImGui::EndTabBar();
                }
                ImGui::End();

                if (showExportMeshesPopup) {
                    ImGui::OpenPopup("Exporting Meshes");
                }
                ImVec2 center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Exporting Meshes", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                    ImGui::Text("Progress: %.1f", static_cast<float>(currentlyExportingMesh) / static_cast<float>(ga->population.size()) * 100.0f);
                    if (!showExportMeshesPopup) ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }

                if (showExportMeshPopup) {
                    ImGui::OpenPopup("Exporting Mesh");
                }
                center = ImGui::GetMainViewport()->GetCenter();
                ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
                if (ImGui::BeginPopupModal("Exporting Mesh", NULL, ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoMove)) {
                    ImGui::Text("Exporting Mesh...");
                    if (!showExportMeshPopup) ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }

                fileDialog.Display();
                populationFileDialog.Display();
                everyMeshFileDialog.Display();
                logFileDialog.Display();

                if(fileDialog.IsOpened())
                {
                    if (fileDialog.HasSelected())
                    {
                        if (!exportMeshFuture.valid() or (exportMeshFuture.valid() and exportMeshFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)) {
                            exportMeshFuture = std::async([&fileDialog, &meshLoader, currentMesh]() {
                                showExportMeshPopup = true;
                                meshLoader->saveMesh(fileDialog.GetSelected().string(), currentMesh);
                                showExportMeshPopup = false;
                            });
                        }
                    }
                }

                if (everyMeshFileDialog.IsOpened()) {
                    if (everyMeshFileDialog.HasSelected()) {
                        if (!exportMeshesFuture.valid() or (exportMeshesFuture.valid() and exportMeshesFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready)) {
                             exportMeshesFuture = std::async([&currentlyExportingMesh, ga, &everyMeshFileDialog, &meshLoader, isRocky, ico, cube, icoSubdivisions, cubeSubdivisions, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale, usePoissonDisk, poissonMinDistance](){
                                showExportMeshesPopup = true;
                                for (int i = 0; i < ga->population.size(); i++)
                                {
                                    currentlyExportingMesh = i;
                                    std::shared_ptr<Geometry::Mesh> mesh;
                                     if (usePoissonDisk)
                                         mesh = Geometry::Mesh::fromPlanetPD(*ga->population[i], poissonMinDistance, Geometry::Mesh::noVertexColor(), false);
                                     else if (cube && isRocky)
                                         mesh = Geometry::Mesh::fromCubePlanetRockyfied(*ga->population[i], cubeSubdivisions, Geometry::Mesh::noVertexColor(), false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                                     else if (ico && isRocky)
                                         mesh = Geometry::Mesh::fromIcoPlanetRockyfied(*ga->population[i], icoSubdivisions, Geometry::Mesh::noVertexColor(), false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                                     else if (isRocky)
                                         mesh = Geometry::Mesh::fromPlanetRockyfied(*ga->population[i], Geometry::Mesh::noVertexColor(), 0.01f, false, rockyFractalOctaves, rockyFractalIntensity);
                                     else if (cube)
                                         mesh = Geometry::Mesh::fromCubePlanet(*ga->population[i], cubeSubdivisions);
                                     else if (ico)
                                         mesh = Geometry::Mesh::fromIcoPlanet(*ga->population[i], icoSubdivisions);
                                     else
                                         mesh = Geometry::Mesh::fromPlanet(*ga->population[i]);
                                    auto path = everyMeshFileDialog.GetSelected().string();
                                    path += "_" + std::to_string(i) + ".obj";
                                    meshLoader->saveMesh(path, mesh);
                                }
                                showExportMeshesPopup = false;
                            });
                        }
                    }
                }

                if(populationFileDialog.IsOpened())
                {
                    if (populationFileDialog.HasSelected())
                    {
                        populationExportPath = populationFileDialog.GetSelected().string();
                        std::cout << populationExportPath << std::endl;

                        auto population = PlanetsPopulation(ga->population, ga->radius(), ga->nMeridians(), ga->nParallels());
                        population.save(populationExportPath);
                    }
                }

                if (logFileDialog.IsOpened()) {
                    if (logFileDialog.HasSelected()) {
                        auto logPath = logFileDialog.GetSelected().string();
                        logPath += ".txt";
                        auto log = ga->log();
                        std::ofstream ofs(logPath);
                        ofs.write(log.data(), log.size());
                        ofs.close();
                    }
                }
            });

        // UPDATE Geometry::MeshES
        if (ga and ga->hasInitialized() and dirtyPlanets)// and !looping)
        {
            // Remove previous renderables
            if (currentMeshRenderID != 0) {
                _renderer->removeRenderable(currentMeshRenderID);
                currentMeshRenderID = 0;
            }
            if (currentMeshMaterialID != 0) {
                _renderer->removeInstanceMaterial(currentMeshMaterialID);
                currentMeshMaterialID = 0;
            }
            for (auto& ids : cpRenderIDs) {
                _renderer->removeRenderable(ids.first);
                _renderer->removeInstanceMaterial(ids.second);
            }
            cpRenderIDs.clear();

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
                    //auto pso = shaded ? "VCPHONG" : "PC";
                    //auto bvhPso = "curve";
                    if (curvatureBasedColoring)
                    {
                        currentMesh = Geometry::Mesh::fromPlanetGaussCurvatureColor(*ga->population[currentPlanet], glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
                    }
                    else if (fitnessBasedColoring)
                    {
                        currentMesh = Geometry::Mesh::fromPlanetFitnessColor(
                            *ga->population[currentPlanet],
                            glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
                            glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
                            tessellationResolution,
                            fitnessBasedColoringDiscrete,
                            fitnessBasedColoringDiscreteTreshold);
                    }
                    else
                    {
                        if (usePoissonDisk) {
                            currentMesh = Geometry::Mesh::fromPlanetPD(*ga->population[currentPlanet], poissonMinDistance, meshColor, false);
                        }
                        else if (cube && isRocky) {
                            currentMesh = Geometry::Mesh::fromCubePlanetRockyfied(*ga->population[currentPlanet], cubeSubdivisions, meshColor, false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                        }
                        else if (ico && isRocky) {
                            currentMesh = Geometry::Mesh::fromIcoPlanetRockyfied(*ga->population[currentPlanet], icoSubdivisions, meshColor, false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                        }
                        else if (isRocky) {
                            currentMesh = Geometry::Mesh::fromPlanetRockyfied(*ga->population[currentPlanet], meshColor, tessellationResolution, false, rockyFractalOctaves, rockyFractalIntensity);
                        } else if (cube) {
                            currentMesh = Geometry::Mesh::fromCubePlanet(*ga->population[currentPlanet], cubeSubdivisions, meshColor);
                        } else if (ico) {
                            currentMesh = Geometry::Mesh::fromIcoPlanet(*ga->population[currentPlanet], icoSubdivisions, meshColor);
                        } else {
                            currentMesh = Geometry::Mesh::fromPlanet(*ga->population[currentPlanet], meshColor, tessellationResolution);
                        }
                    }
                }
                else
                {
                    if (usePoissonDisk) {
                        currentMesh = Geometry::Mesh::fromPlanetPD(*ga->population[currentPlanet], poissonMinDistance, meshColor, false);
                    }
                    else if (cube && isRocky) {
                        currentMesh = Geometry::Mesh::fromCubePlanetRockyfied(*ga->population[currentPlanet], cubeSubdivisions, meshColor, false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                    }
                    else if (ico && isRocky) {
                        currentMesh = Geometry::Mesh::fromIcoPlanetRockyfied(*ga->population[currentPlanet], icoSubdivisions, meshColor, false, rockyFractalOctaves, rockyFractalIntensity, rockyFractalScale);
                    }
                    else if (isRocky) {
                        currentMesh = Geometry::Mesh::fromPlanetRockyfied(*ga->population[currentPlanet], meshColor, tessellationResolution, false, rockyFractalOctaves, rockyFractalIntensity);
                    } else if (cube) {
                        currentMesh = Geometry::Mesh::fromCubePlanet(*ga->population[currentPlanet], cubeSubdivisions, meshColor);
                    } else if (ico) {
                        currentMesh = Geometry::Mesh::fromIcoPlanet(*ga->population[currentPlanet], icoSubdivisions, meshColor);
                    } else {
                        currentMesh = Geometry::Mesh::fromPlanet(*ga->population[currentPlanet], meshColor, tessellationResolution);
                    }
                }

                if (currentMesh) {
                    currentMeshRenderID = _renderer->addRenderable(*currentMesh, {});
                    auto pso = shaded ? "VCWARD" : "Unlit";
                    currentMeshMaterialID = _renderer->addDefaultInstanceMaterial(pso);
                    _renderer->setInstanceMaterial(currentMeshMaterialID, getBytes(0.25f), ROUGHNESS);
                }

#if SHOW_BVH
                auto bvh = BVH(currentMesh->getVertices(), currentMesh->getFacesData(), 16);
                auto bvhLines = bvh.getLines();
                auto bvhMesh = Geometry::Mesh::fromPolygon(bvhLines, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), false);
                auto bvhId = _renderer->addRenderable(*bvhMesh, Rendering::psoConfigs.at("curve"), {}, Rendering::RenderLayer::OPAQUE);
                meshIDs.push_back(bvhId);
#endif

                //meshIDs.push_back(ids);
            }

            // draw normal sticks
            if (normals)
            {
                /*
                auto normalSticks = ga->population[currentPlanet]->normalSticks(normalLength, normalStep);
                auto sticksMesh = Geometry::Mesh::fromPolygon(normalSticks, normalColor, false);
                auto ids = _renderer->addRenderable(
                                                         *sticksMesh,
                                                         Rendering::psoConfigs.at("curve"), {},
                                                         Rendering::RenderLayer::OPAQUE
                                                         );
                meshIDs.push_back(ids);
                */
            }
            // draw control polyhedron
            if (showControlPolyhedron)
            {
                cpMeshes.clear();
                for (const auto & i : ga->population[currentPlanet]->parallelsCP())
                {
                    auto mesh = Geometry::Mesh::fromPolygon(i, glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), true);
                    auto rID = _renderer->addRenderable(*mesh, {});
                    auto mID = _renderer->addDefaultInstanceMaterial("curve");
                    cpRenderIDs.push_back({rID, mID});
                    cpMeshes.push_back(mesh);
                }
                // draw meridians
                for (int i = 0; i < ga->population[currentPlanet]->meridiansCP().size(); i++)
                {
                    auto mesh = Geometry::Mesh::fromPolygon(ga->population[currentPlanet]->meridiansCP()[i], glm::vec4(0.0f, 0.0f, 1.0f, 1.0f), true);
                    auto rID = _renderer->addRenderable(*mesh, {});
                    auto mID = _renderer->addDefaultInstanceMaterial("curve");
                    cpRenderIDs.push_back({rID, mID});
                    cpMeshes.push_back(mesh);
                }
            }
            else
            {
                cpMeshes.clear();
            }
        }

        RenderQueue renderQueue;

        if (currentMeshRenderID != 0 && showMesh)
        {
            auto pso = shaded ? "VCWARD" : "Unlit";
            renderQueue.add(RenderItem(currentMeshRenderID, currentMeshMaterialID, false, wireframe), pso, Rendering::RenderLayer::OPAQUE);
        }

        if (showControlPolyhedron) {
            for (auto& ids : cpRenderIDs) {
                renderQueue.add(RenderItem(ids.first, ids.second, false, false), "curve", Rendering::RenderLayer::OPAQUE);
            }
        }

        // LOOPING ON SECOND THREAD
        if (ga and ga->hasInitialized() and gaShouldLoop and !ga->shouldTerminate()) {
            if (gaShouldLoop) {
                //std::cout << "Running GA loop..." << std::endl;
                // launch async loop
                if (!voidFuture.valid()) voidFuture = std::async(std::launch::async, [&ga, &looping, &dirtyPlanets]() {
                    looping = true;
                    ga->loop();
                    looping = false;
                    dirtyPlanets = true;
                });
                if (voidFuture.valid() && voidFuture.wait_for(std::chrono::seconds(0)) == std::future_status::ready) {
                    voidFuture = std::async(std::launch::async, [&ga, &looping, &dirtyPlanets]() {
                        looping = true;
                        ga->loop();
                        looping = false;
                        dirtyPlanets = true;
                    });
                }
            }
        }


        const int trianglesPerMeshlet = 32;
        int numMeshlets = (meshShaderTestMesh->getNumFaces() + trianglesPerMeshlet - 1) / trianglesPerMeshlet;
        if (numMeshlets == 0) numMeshlets = 1;

        /*
        _renderer->iDraw(
            *meshShaderTestMesh,
            {},
            "TestMesh",
            {}, false, wireframe,
            numMeshlets, 1, 1,
            32, 1, 1
            );
            */

        // Dynamic Procedural Icosphere with Object & Mesh Shaders
        std::vector<std::pair<MaterialType, std::vector<std::byte>>> icosphereMaterials;

        // Update current planet data if it exists
        std::shared_ptr<Planet> activePlanet = nullptr;
        if (ga && ga->hasInitialized() && !ga->population.empty()) {
            activePlanet = ga->population[currentPlanet];
        }

        if (activePlanet) {
            auto cp = activePlanet->parallelsCP();
            if (!cp.empty() && !cp[0].empty()) {
                std::vector<glm::vec3> flatCP;
                for (auto& row : cp) {
                    for (auto& p : row) flatCP.push_back(p);
                }

                std::vector<std::byte> cpBytes(flatCP.size() * sizeof(glm::vec3));
                std::memcpy(cpBytes.data(), flatCP.data(), cpBytes.size());
                //icosphereMaterials.push_back({PLANET_CP, cpBytes});

                auto knotsU = activePlanet->knotsU();
                std::vector<std::byte> kuBytes(knotsU.size() * sizeof(int));
                std::memcpy(kuBytes.data(), knotsU.data(), kuBytes.size());
                //icosphereMaterials.push_back({PLANET_KNOTS_U, kuBytes});

                auto knotsV = activePlanet->knotsV();
                std::vector<std::byte> kvBytes(knotsV.size() * sizeof(int));
                std::memcpy(kvBytes.data(), knotsV.data(), kvBytes.size());
                //icosphereMaterials.push_back({PLANET_KNOTS_V, kvBytes});

                PlanetInfoMaterial info;
                info.degreeU = activePlanet->degreeU();
                info.degreeV = activePlanet->degreeV();
                info.nCP_U = cp[0].size();
                info.nCP_V = cp.size();
                info.planetRadius = radius; // Default radius for icosphere
                info.usePositionTexture = usePositionTexture_UI;
                info.useNormalTexture = useNormalTexture_UI;
                info.constantLOD = constantLOD_UI;
                info.useConstantLOD = useConstantLOD_UI;
                info.showMesh = showMesh_UI;
                info.isRocky = isRocky_UI;
                info.fractalOctaves = fractalOctaves_UI;
                info.fractalIntensity = fractalIntensity_UI;
                info.fractalScale = fractalScale_UI;
                info.useRockyTexture = useRockyTexture_UI;
                //icosphereMaterials.push_back({PLANET_INFO, getBytes(info)});

                CompactPlanetInfoMaterial cinfo;
                cinfo.fractalIntensity = fractalIntensity_UI;
                cinfo.fractalScale = fractalScale_UI;
                cinfo.planetRadius = radius;
                icosphereMaterials.push_back({COMPACT_PLANET_INFO, getBytes(cinfo)});

                // ADD PLANET TEXTURE
                static std::shared_ptr<Core::Texture> planetTex = nullptr;
                static std::shared_ptr<Core::Texture> planetNormalTex = nullptr;
                
                if (planetTex == nullptr && usePositionTexture_UI) planetTex = activePlanet->toTexture(512, 512);
                if (planetNormalTex == nullptr && useNormalTexture_UI) planetNormalTex = activePlanet->toNormalTexture(512, 512);

                static std::shared_ptr<Core::Texture> rockyTexTL = nullptr;
                static std::shared_ptr<Core::Texture> rockyTexTR = nullptr;
                static std::shared_ptr<Core::Texture> rockyTexBL = nullptr;
                static std::shared_ptr<Core::Texture> rockyTexBR = nullptr;

                static uint64_t planetRenderableID = 0;
                static uint64_t planetMaterialID = 0;

                if (dirtyPlanets) {
                    auto size = 512;
                    if (usePositionTexture_UI) planetTex = activePlanet->toTexture(size, size);
                    else planetTex = nullptr;
                    
                    if (useNormalTexture_UI) planetNormalTex = activePlanet->toNormalTexture(size, size);
                    else planetNormalTex = nullptr;
                    
                    if (useRockyTexture_UI) {
                        std::cout << "[DEBUG] Generating rocky textures (4x 16k) via GPU kernel..." << std::endl;

                        auto w = 16384 / 4;
                        rockyTexTL = std::make_shared<Core::Texture>(std::vector<uint8_t>(w * w * sizeof(float), 0), w, w, Core::R32F, sizeof(float), Core::RockyNoise_TL);
                        rockyTexTR = std::make_shared<Core::Texture>(std::vector<uint8_t>(w * w * sizeof(float), 0), w, w, Core::R32F, sizeof(float), Core::RockyNoise_TR);
                        rockyTexBL = std::make_shared<Core::Texture>(std::vector<uint8_t>(w * w * sizeof(float), 0), w, w, Core::R32F, sizeof(float), Core::RockyNoise_BL);
                        rockyTexBR = std::make_shared<Core::Texture>(std::vector<uint8_t>(w * w * sizeof(float), 0), w, w, Core::R32F, sizeof(float), Core::RockyNoise_BR);
                        auto generateQuadrant = [&](std::shared_ptr<Core::Texture> tex, int qX, int qY) {
                            PlanetInfoMaterial qInfo = info;
                            qInfo.quadrantX = qX;
                            qInfo.quadrantY = qY;
                            _renderer->compute(
                                "RockyTextureGenerator",
                                {},
                                {rockyTexTL, rockyTexTR, rockyTexBL, rockyTexBR},
                                {
                                    {PLANET_INFO, getBytes(qInfo)},
                                    {PLANET_CP, cpBytes},
                                    {PLANET_KNOTS_U, kuBytes},
                                    {PLANET_KNOTS_V, kvBytes}
                                },
                                {w, w, 1},
                                {32, 32, 1}
                            );
                        };

                        generateQuadrant(rockyTexTL, 0, 0);
                        generateQuadrant(rockyTexTR, 1, 0);
                        generateQuadrant(rockyTexBL, 0, 1);
                        generateQuadrant(rockyTexBR, 1, 1);
                    }
                    else
                    {
                        rockyTexTL = nullptr;
                        rockyTexTR = nullptr;
                        rockyTexBL = nullptr;
                        rockyTexBR = nullptr;
                    }
                    if (planetRenderableID != 0) {
                        _renderer->removeRenderable(planetRenderableID);
                        std::vector<std::shared_ptr<Core::Texture>> textures;
                        if (planetTex) textures.push_back(planetTex);
                        if (planetNormalTex) textures.push_back(planetNormalTex);
                        if (useRockyTexture_UI) {
                            textures.push_back(rockyTexTL);
                            textures.push_back(rockyTexTR);
                            textures.push_back(rockyTexBL);
                            textures.push_back(rockyTexBR);
                        }
                        planetRenderableID = _renderer->addRenderable(
                            *meshShaderTestMesh,
                            textures
                            );
                    }
                }

                if (planetRenderableID == 0) {
                    std::vector<std::shared_ptr<Core::Texture>> textures;
                    if (planetTex) textures.push_back(planetTex);
                    if (planetNormalTex) textures.push_back(planetNormalTex);
                    if (useRockyTexture_UI) {
                        textures.push_back(rockyTexTL);
                        textures.push_back(rockyTexTR);
                        textures.push_back(rockyTexBL);
                        textures.push_back(rockyTexBR);
                    }
                    planetRenderableID = _renderer->addRenderable(*meshShaderTestMesh, textures);
                }
                if (planetMaterialID == 0) {
                    //planetMaterialID = _renderer->addDefaultInstanceMaterial("DynamicIcosphere");
                    planetMaterialID = _renderer->addDefaultInstanceMaterial("PlanetShader");
                }

                // update materials
                for (const auto& mat : icosphereMaterials) {
                    _renderer->setInstanceMaterial(planetMaterialID, mat.second, mat.first);
                }
                renderQueue.add(RenderItem(planetRenderableID, planetMaterialID, false, wireframe, {5120, 1, 1}, {64, 1, 1}), "PlanetShader", Rendering::RenderLayer::OPAQUE);
                //renderQueue.add(RenderItem(planetRenderableID, planetMaterialID, false, wireframe, 5120, 1, 1, 64, 1, 1), "DynamicIcosphere", Rendering::RenderLayer::OPAQUE);
            }
        }

        // DRAW AXES
        if (xMesh) _renderer->iDraw(*xMesh, {}, "curve", {});
        if (yMesh) _renderer->iDraw(*yMesh, {}, "curve", {});
        if (zMesh) _renderer->iDraw(*zMesh, {}, "curve", {});

        //_renderer->iDraw(*icoMesh, {}, "VCWARD", {});

        // set global materials
        Lights lights;
        lights.globalAmbientLightColor = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
        lights.numDirectionalLights = 1;
        auto lightDirection = glm::inverse(camera.getViewMatrix()) * glm::vec4(0.0f, 0.0f, -1.0f, 0.0f);
        lights.directionalLights[0].direction = lightDirection;
        lights.directionalLights[0].color = glm::vec4(2.0f, 2.0f, 2.0f, 1.0f);
        _renderer->setGlobalMaterial(getBytes(lights), LIGHTS);

        _renderer->setGlobalMaterial(getBytes(camera.getViewMatrix()), VIEW_MATRIX);
        _renderer->setGlobalMaterial(getBytes(glm::vec4(camera.getPosition(), 1.0f)), CAMERA_POSITION);

        auto drawableSize = _renderer->getDrawableSize();
        auto aspectRatio = drawableSize[0] * viewport[2] / (drawableSize[1] * viewport[3]);
        auto projectionMatrix = glm::perspectiveRH_ZO(
                    glm::radians(65.0f), aspectRatio, 0.1f, 1000.0f
            );
        _renderer->setGlobalMaterial(getBytes(projectionMatrix), PROJECTION_MATRIX);
        _renderer->setGlobalMaterial(getBytes(projectionMatrix * camera.getViewMatrix()), VIEW_PROJECTION_MATRIX);

        dirtyPlanets = false;
        _renderer->update(renderQueue, viewport);
    }
    }
    SDL_DestroyWindow(_window);
    std::cout << "Exiting from app::run()" << std::endl;
}

//#endif
