cmake_minimum_required(VERSION 3.9)
cmake_policy(SET CMP0079 NEW)

if(APPLE)
    link_directories("/opt/homebrew/opt/libomp/lib/")
    include_directories("/opt/homebrew/opt/libomp/include/")
endif()

set(CMAKE_BUILD_RPATH APPEND /Library/Frameworks/)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")
include(MetalShaderSupport)

project(evolving_planets)
enable_language(Metal)

set(CMAKE_CXX_STANDARD 20)

# Configurazioni per evitare problemi di code signing
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO)
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "")
set(CMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED NO)


file(GLOB_RECURSE SOURCE_FILES src/*.cpp)
file(GLOB_RECURSE HEADER_FILES include/*.hpp)
file(GLOB_RECURSE RESOURCES resources/*)
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
find_package(SDL2_ttf REQUIRED
)
if (SDL2_ttf_FOUND)
    message(STATUS "SDL_ttf FOUND")
    message(STATUS "SDL2_ttf includes: ${SDL2_TTF_INCLUDE_DIRS}")
    message(STATUS "SDL2_ttf libraries: ${SDL2_TTF_LIBRARIES}")
endif()
target_include_directories(main PRIVATE ${SDL2_TTF_INCLUDE_DIRS})
target_link_libraries(main PRIVATE ${SDL2_TTF_LIBRARIES})

add_subdirectory(assimp)
add_subdirectory(glm)
add_subdirectory(imgui)
add_subdirectory(bezier)

if(APPLE)
    message("setting openmp on macos")
    set(OpenMP_C_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_C_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY omp)

    set(OpenMP_CXX_FLAGS "-Xpreprocessor -fopenmp")
    set(OpenMP_CXX_LIB_NAMES "omp")
    set(OpenMP_omp_LIBRARY omp)
endif()
find_package(OpenMP REQUIRED)
if(OpenMP_CXX_FOUND)
    message(STATUS "OpenMP found")
    endif()

add_subdirectory(gravity)
target_embed_metal_shader_libraries(main GravityShaders)
add_custom_command(TARGET main POST_BUILD
        DEPENDS GravityShaders
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:GravityShaders>" "$<TARGET_BUNDLE_CONTENT_DIR:main>/Resources/$<TARGET_FILE_NAME:GravityShaders>"
        VERBATIM
)

add_subdirectory(genetic)

target_include_directories(imgui PRIVATE ${SDL2_TTF_INCLUDE_DIRS})

target_include_directories(main PUBLIC
        ./include
        Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
)
target_include_directories(imgui PUBLIC
        ./include
        Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
)

set_target_properties(assimp PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
)
if(TARGET glm)
    set_target_properties(glm PROPERTIES
            XCODE_ATTRIBUTE_CODE_SIGNING_REQUIRED NO
            XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY ""
    )
endif()

target_link_libraries(main PRIVATE
        "-framework Metal"
        "-framework Foundation"
        "-framework QuartzCore"
        "-framework MetalKit"
        "-framework AppKit"
        SDL2::SDL2
        SDL2_ttf::SDL2_ttf
        assimp::assimp
        glm::glm
        imgui
        bezier
        gravity
        genetic
)

set_target_properties(main PROPERTIES
        BUILD_RPATH "/Library/Frameworks"
)


# SET UP FOR XCODE
file(GLOB_RECURSE IMPLEMENTATION_FILES LIST_DIRECTORIES true
        src/*.cpp
        shaders/*.metal
        )
file(GLOB_RECURSE HEADER_FILES LIST_DIRECTORIES true
        include/*.hpp
        )
file(GLOB_RECURSE RESOURCES LIST_DIRECTORIES true
        resources/*
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${IMPLEMENTATION_FILES}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}"
        FILES ${HEADER_FILES})

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        FILES ${SHADER_FILES}
)
source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}/resources"
        FILES ${RESOURCES}
)