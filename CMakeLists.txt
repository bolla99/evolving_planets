cmake_minimum_required(VERSION 3.26)

project(evolving_planets)

set(CMAKE_CXX_STANDARD 17)

add_executable(main
        src/main.cpp
        src/App.cpp
        src/renderer.cpp
)

find_package(SDL2 REQUIRED)

target_include_directories(main PUBLIC
        ./include
)

target_link_libraries(main PUBLIC
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
        SDL2::SDL2
)

set_target_properties(main PROPERTIES
        BUILD_RPATH "/Library/Frameworks"
)