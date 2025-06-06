cmake_minimum_required(VERSION 3.26)

file(GLOB SHADERS *.metal)
add_metal_shader_library(Shaders
        STANDARD macos-metal2.4
        ${SHADERS}
)