cmake_minimum_required(VERSION 3.19)

set(CMAKE_BUILD_RPATH APPEND /Library/Frameworks/)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
include(MetalShaderSupport)

project(evolving_planets)
enable_language(Metal)

set(CMAKE_CXX_STANDARD 20)

file(GLOB SOURCE_FILES src/*.cpp)
file(GLOB HEADER_FILES include/*.hpp)
file(GLOB RESOURCES resources/*)
set_source_files_properties(${RESOURCES} PROPERTIES HEADER_FILE_ONLY TRUE)
set_source_files_properties(${RESOURCES}
       PROPERTIES
        MACOSX_PACKAGE_LOCATION "Resources"
)

add_executable(main MACOSX_BUNDLE
        ${SOURCE_FILES}
        ${HEADER_FILES}
        ${RESOURCES}
)

add_subdirectory(shaders)
target_embed_metal_shader_libraries(main
        Shaders
)

add_custom_command(TARGET main POST_BUILD
        DEPENDS Shaders
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Shaders>" "$<TARGET_BUNDLE_CONTENT_DIR:main>/Resources/$<TARGET_FILE_NAME:Shaders>"
        VERBATIM
)

find_package(SDL2 REQUIRED)
add_subdirectory(assimp)

target_include_directories(main PUBLIC
        ./include
        Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
)

target_link_libraries(main PRIVATE
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework MetalKit"
        "-framework AppKit"
        SDL2::SDL2
        assimp::assimp
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