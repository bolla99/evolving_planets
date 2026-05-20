cmake_minimum_required(VERSION 3.26)

file(GLOB SHADERS *.metal)
file(GLOB SHADER_HEADERS *.hpp)

add_metal_shader_library(Shaders
        STANDARD metal3.0
        ${SHADERS}
)

# Mostra anche gli header in Xcode e li associa al target.
# Nota: niente virgole tra le liste di file.
target_sources(Shaders PUBLIC
        ${SHADERS}
        ${SHADER_HEADERS}
)

target_include_directories(Shaders PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})