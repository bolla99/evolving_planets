#include <iostream>

#define NS_PRIVATE_IMPLEMENTATION
#define CA_PRIVATE_IMPLEMENTATION
#define MTL_PRIVATE_IMPLEMENTATION

#include "app.hpp"

void LogSDLError();

int main()
{
    try
    {
        app(800, 600).run();
    }
    catch (const std::exception& e)
    {
        std::cerr << e.what() << std::endl;
    }
    std::cout << "Exiting from end of main" << std::endl;
}



