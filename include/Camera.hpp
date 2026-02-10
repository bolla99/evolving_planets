//
// Created by Giovanni Bollati on 21/06/25.
//

#ifndef CAMERA_HPP
#define CAMERA_HPP

#include <glm/glm.hpp>

#define GLM_ENABLE_EXPERIMENTAL

#include <glm/gtx/quaternion.hpp>
#include <glm/ext/matrix_transform.hpp>

class Camera
{
public:
    Camera() :
    position(0.0f, 0.0f, 0.0f),
    orientation(1.0f, 0.0f, 0.0f, 0.0f),
    fov(60.0f),
    nearPlane(0.1f),
    farPlane(1000.0f) {}
    virtual ~Camera() = default;

    glm::vec3 position;
    glm::quat orientation;
    float fov;
    float nearPlane;
    float farPlane;

    [[nodiscard]] virtual glm::mat4x4 getViewMatrix() const
    {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), position) * glm::toMat4(glm::normalize(orientation))
        );
    }

    void lookAt(glm::vec3 target, glm::vec3 upDirection = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        orientation = glm::quatLookAt(target - position, upDirection);
    }

    [[nodiscard]] glm::vec3 front() const { return orientation * glm::vec3(0.0f, 0.0f, -1.0f); }
    [[nodiscard]] glm::vec3 right() const { return orientation * glm::vec3(1.0f, 0.0f, 0.0f); }
    [[nodiscard]] glm::vec3 up() const { return orientation * glm::vec3(0.0f, 1.0f, 0.0f); }

    void advance(float distance)
    {
        if (std::isnan(distance)) return;
        position -= orientation * glm::vec3(0.0f, 0.0f, distance);
    }
    void strafe(float distance)
    {
        if (std::isnan(distance)) return;
        position += orientation * glm::vec3(distance, 0.0f, 0.0f);
    }

    virtual void pan(float amount) = 0;
    virtual void tilt(float amount) = 0;
    virtual void zoom(float amount) = 0;
};

class FPSCamera : public Camera
{
public:
    void pan(float amount) override
    {
        orientation = glm::normalize(glm::angleAxis(amount, glm::vec3(0.0f, 1.0f, 0.0f)) * orientation);
    }
    void tilt(float amount) override
    {
        const glm::vec3 right = orientation * glm::vec3(1.0f, 0.0f, 0.0f);
        orientation = glm::normalize(glm::angleAxis(amount, right) * orientation);
    }
    void zoom(float amount) override {}
};

#endif //CAMERA_HPP
