#include <iostream>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "App.hpp"
#include "Rendering/EvolvingPlanetsApp.hpp"
#include "PreCompileSettings.hpp"
#include "RTGPApp.hpp"

int main()
{
    try
    {
#if APP == 0
        auto app = EvolvingPlanetsApp(800, 600);
#elif APP == 1
        auto app = RTGPApp(800, 600);
#endif
        app.init();
        app.run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    std::cout << "Exiting from end of main" << std::endl;
}



