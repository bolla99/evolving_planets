

file(GLOB_RECURSE SOURCE_FILES *.cpp *.mm)
file(GLOB_RECURSE HEADER_FILES *.hpp *.h)

add_library(bezier STATIC
    ${SOURCE_FILES}
    ${HEADER_FILES}
)

target_include_directories(
    bezier PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include ./
)

target_link_libraries(bezier PUBLIC
        glm::glm
)