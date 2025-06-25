//
// Created by Giovanni Bollati on 21/06/25.
//

#include <glm/glm.hpp>
#include <Camera.hpp>

#include "glm/ext/matrix_transform.hpp"

glm::mat4x4 Camera::getViewMatrix() const
{
    return glm::lookAt(
        position,
        position - (glm::normalize(glm::cross(right, up))), // Look at the right direction
        up // Up vector
    );
}
