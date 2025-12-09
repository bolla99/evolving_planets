cmake_minimum_required(VERSION 3.26)
cmake_policy(SET CMP0079 NEW)
cmake_policy(SET CMP0175 NEW)

project(evolving_planets)

if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "15.0")
    link_directories("/opt/homebrew/opt/libomp/lib/")
    include_directories("/opt/homebrew/opt/libomp/include/")
endif()

#set(CMAKE_BUILD_RPATH APPEND /Library/Frameworks/)
set(CMAKE_MODULE_PATH "${CMAKE_MODULE_PATH};${CMAKE_SOURCE_DIR}/cmake")

include(MetalShaderSupport)

set(CMAKE_CXX_STANDARD 20)

# Imposta il path al compilatore Metal
set(CMAKE_Metal_COMPILER "/Applications/Xcode.app/Contents/Developer/Toolchains/Metal.xctoolchain/usr/bin/metal" CACHE FILEPATH "Path to Metal compiler")
enable_language(Metal)

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

set_target_properties(main PROPERTIES
    INSTALL_RPATH "@executable_path/../Frameworks"
)

add_subdirectory(shaders)
target_embed_metal_shader_libraries(main
        Shaders
)
add_custom_command(TARGET main POST_BUILD
        #DEPENDS Shaders
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:Shaders>" "$<TARGET_BUNDLE_CONTENT_DIR:main>/Resources/$<TARGET_FILE_NAME:Shaders>"
        VERBATIM
)

find_package(SDL2 REQUIRED)
message(STATUS "Contenuto di SDL2_LIBRARY: ${SDL2_LIBRARY}")
find_package(SDL2_ttf REQUIRED
)
if (SDL2_ttf_FOUND)
    message(STATUS "SDL_ttf FOUND")
endif()

set(ASSIMP_BUILD_SHARED_LIBS OFF CACHE BOOL "Build ASSIMP as static libraries" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build all libraries as static" FORCE)
add_subdirectory(assimp)
add_subdirectory(glm)
#find_package(glm CONFIG REQUIRED)



add_subdirectory(imgui)
add_subdirectory(bezier)
target_include_directories(
        bezier PUBLIC
        ./include
        Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
)
target_embed_metal_shader_libraries(main BezierShaders)
add_custom_command(TARGET main POST_BUILD
        #DEPENDS BezierShaders
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:BezierShaders>" "$<TARGET_BUNDLE_CONTENT_DIR:main>/Resources/$<TARGET_FILE_NAME:BezierShaders>"
        VERBATIM
)

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
        #DEPENDS GravityShaders
        COMMAND ${CMAKE_COMMAND} -E copy "$<TARGET_FILE:GravityShaders>" "$<TARGET_BUNDLE_CONTENT_DIR:main>/Resources/$<TARGET_FILE_NAME:GravityShaders>"
        VERBATIM
)

add_subdirectory(genetic)
target_include_directories(
        genetic PUBLIC
        ./include
        Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk/usr/include
)
target_link_libraries(genetic PUBLIC gravity)

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

target_link_libraries(bezier PUBLIC gravity)

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

target_include_directories(main PRIVATE ${GLM_INCLUDE_DIR})

set(SDL2_DYLIB_PATH "/Library/Frameworks/SDL2.framework/Versions/A/SDL2")
set(SDL2_ttf_DYLIB_PATH "/Library/Frameworks/SDL2_ttf.framework/Versions/A/SDL2_ttf")

# --- BLOCCO CORRETTO PER SDL2_ttf ---
set(SDL2_ttf_FRAMEWORK_DIR_SRC "/Library/Frameworks/SDL2_ttf.framework")

add_custom_command(
        TARGET main POST_BUILD

        # 1. Crea la cartella Frameworks
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:main>/../Frameworks/"

        # 2. *** COPIA L'INTERO FRAMEWORK ***
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SDL2_ttf_FRAMEWORK_DIR_SRC}" # Copia l'intera cartella .framework
        "$<TARGET_FILE_DIR:main>/../Frameworks/SDL2_ttf.framework" # Destinazione: crea la struttura interna

        # 3. Correzione del percorso (install_name_tool) - Deve rimanere
        #COMMAND install_name_tool -change
        #@rpath/SDL2_ttf.framework/Versions/A/SDL2_ttf
        #@loader_path/../Frameworks/SDL2_ttf.framework/Versions/A/SDL2_ttf
        #"$<TARGET_FILE:main>"
        #COMMENT "Copia e corregge SDL2_ttf.framework nel bundle"
)

# --- BLOCCO CORRETTO PER SDL2_ttf ---
set(SDL2_FRAMEWORK_DIR_SRC "/Library/Frameworks/SDL2.framework")

add_custom_command(
        TARGET main POST_BUILD

        # 1. Crea la cartella Frameworks
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:main>/../Frameworks/"

        # 2. *** COPIA L'INTERO FRAMEWORK ***
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${SDL2_FRAMEWORK_DIR_SRC}" # Copia l'intera cartella .framework
        "$<TARGET_FILE_DIR:main>/../Frameworks/SDL2.framework" # Destinazione: crea la struttura interna

        # 3. Correzione del percorso (install_name_tool) - Deve rimanere
        #COMMAND install_name_tool -change
        #@rpath/SDL2.framework/Versions/A/SDL2
        #@loader_path/../Frameworks/SDL2.framework/Versions/A/SDL2
        #"$<TARGET_FILE:main>"
)



#set_target_properties(main PROPERTIES
#        BUILD_RPATH "/Library/Frameworks"
#)


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
