cmake_minimum_required(VERSION 3.26)

project(evolving_planets)

set(CMAKE_CXX_STANDARD 17)

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB HEADER_FILES include/*.hpp)
file(GLOB SHADERS shaders/*.metal)

add_executable(main
        ${SOURCE_FILES}
        ${HEADER_FILES}
        ${SHADERS}
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


# SET UP FOR XCODE
file(GLOB ALL_FILES
        src/*.cpp
        include/*.hpp
        shaders/*.metal
        )
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        PREFIX "Implementation"
        FILES ${ALL_FILES})