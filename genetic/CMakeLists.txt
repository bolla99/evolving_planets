
add_library(
        genetic STATIC genetic.cpp genetic.hpp cma.cpp
)

target_include_directories(genetic PUBLIC ./)

target_link_libraries(
        genetic PUBLIC
        glm::glm
        bezier
)