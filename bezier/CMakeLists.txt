file(GLOB_RECURSE SOURCE_FILES *.cpp *.mm)
file(GLOB_RECURSE HEADER_FILES *.hpp *.h)

add_library(bezier STATIC
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

set_target_properties(bezier PROPERTIES LINKER_LANGUAGE CXX)

target_include_directories(
    bezier PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include ./
)

target_link_libraries(bezier PUBLIC
        glm::glm
)

cmake_minimum_required(VERSION 3.26)

add_metal_shader_library(BezierShaders
        STANDARD macos-metal2.4
        TriangleIntersection.metal
)