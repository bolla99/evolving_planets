//
// Created by Giovanni Bollati on 21/06/25.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP
#include <array>
#include <glm/glm.hpp>

#include "glm/ext/matrix_transform.hpp"

class Camera
{
public:
    virtual ~Camera() = default;
    glm::vec3 position;
    glm::vec3 up;
    glm::vec3 right;

    virtual glm::mat4x4 getViewMatrix() const;

    void lookAt(glm::vec3 target, glm::vec3 upDirection = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        up = glm::normalize(upDirection);
        right = glm::normalize(glm::cross(up, position - target));
        up = glm::normalize(glm::cross(position - target, right));
    }

    static glm::mat4x4 identityMatrix()
    {
        return glm::lookAt(
                glm::vec3(0.0f, 0.0f, 0.0f), // position
                glm::vec3(0.0f, 0.0f, -1.0f), // target
                glm::vec3(0.0f, 1.0f, 0.0f)  // up direction
            );
    }
};

#endif //CAMERA_HPP
