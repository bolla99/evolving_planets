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
    fov(60.0f),
    nearPlane(0.8f),
    farPlane(1000.0f) {}
    virtual ~Camera() = default;

    float fov;
    float nearPlane;
    float farPlane;

    // getters
    [[nodiscard]] virtual glm::mat4x4 getViewMatrix() const = 0;
    [[nodiscard]]  virtual glm::vec3 getPosition() const = 0;
    virtual glm::quat getOrientation() = 0;


    [[nodiscard]] virtual glm::vec3 front() const = 0;
    [[nodiscard]] virtual glm::vec3 right() const = 0;
    [[nodiscard]] virtual glm::vec3 up() const = 0;

    // helpers for setting camera
    virtual void pan(float amount) = 0;
    virtual void tilt(float amount) = 0;
    virtual void roll(float amount) = 0;

    virtual void alignTo(glm::vec3 direction, glm::vec3 up) = 0;
};

class FPSCamera : public Camera
{
public:
    void pan(float amount) override
    {
        _orientation = glm::normalize(glm::angleAxis(amount, glm::vec3(0.0f, 1.0f, 0.0f)) * _orientation);
    }
    void localPan(float amount)
    {
        _orientation = glm::normalize(glm::angleAxis(amount, up()) * _orientation);
    }
    void tilt(float amount) override
    {
        const glm::vec3 right = _orientation * glm::vec3(1.0f, 0.0f, 0.0f);
        _orientation = glm::normalize(glm::angleAxis(amount, right) * _orientation);
    }
    void roll(float amount) override
    {
        _orientation = glm::normalize(glm::angleAxis(amount, front()) * _orientation);
    }


    void lookAt(glm::vec3 target, glm::vec3 upDirection = glm::vec3(0.0f, 1.0f, 0.0f))
    {
        _orientation = glm::quatLookAt(target - _position, upDirection);
    }

    void setPosition(glm::vec3 position) { _position = position; }
    void setOrientation(glm::quat orientation) { _orientation = orientation; }

    [[nodiscard]] glm::vec3 front() const override { return _orientation * glm::vec3(0.0f, 0.0f, -1.0f); }
    [[nodiscard]] glm::vec3 right() const override { return _orientation * glm::vec3(1.0f, 0.0f, 0.0f); }
    [[nodiscard]] glm::vec3 up() const override { return _orientation * glm::vec3(0.0f, 1.0f, 0.0f); }

    void advance(float distance)
    {
        if (std::isnan(distance)) return;
        _position -= _orientation * glm::vec3(0.0f, 0.0f, distance);
    }
    void strafe(float distance)
    {
        if (std::isnan(distance)) return;
        _position += _orientation * glm::vec3(distance, 0.0f, 0.0f);
    }

    [[nodiscard]] glm::mat4x4 getViewMatrix() const override
    {
        return glm::inverse(
            glm::translate(glm::mat4(1.0f), _position) * glm::toMat4(glm::normalize(_orientation))
        );
    }

    [[nodiscard]] glm::vec3 getPosition() const override { return _position; }
     [[nodiscard]] glm::quat getOrientation() override { return _orientation; }

    void alignTo(glm::vec3 direction, glm::vec3 up) override
    {
        _orientation = glm::quatLookAt(direction, up);
    }

    // order: lowLeft, lowRight, topLeft, topRight
    std::array<glm::vec3, 4> frameInWorldSpace(float aspectRatio, float padding)
    {
        glm::mat4 projection = glm::perspectiveRH_ZO(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        glm::mat4 viewProjInv = glm::inverse(projection * getViewMatrix());

        std::array<glm::vec2, 4> clipPoints = {
            glm::vec2(-1.0f, -1.0f), // lowLeft  (0,0) in clip space
            glm::vec2( -1.0f, 1.0f), // lowRight (1,0) in clip space
            glm::vec2(1.0f,  -1.0f), // topLeft  (0,1) in clip space
            glm::vec2( 1.0f,  1.0f)  // topRight (1,1) in clip space
        };

        std::array<glm::vec3, 4> worldPoints{};
        for (size_t i = 0; i < 4; ++i)
        {
            glm::vec4 pointClip = glm::vec4(clipPoints[i].x, clipPoints[i].y, 0.0f, 1.0f);
            glm::vec4 pointWorld = viewProjInv * pointClip;
            worldPoints[i] = glm::vec3(pointWorld) / pointWorld.w;

            worldPoints[i] += (worldPoints[i] - getPosition()) * padding;
        }

        return worldPoints;
    }

private:
    glm::vec3 _position = {0.0f, 0.0f, 0.0f};
    glm::quat _orientation = {1.0f, 0.0f, 0.0f, 0.0f};
};

#endif //CAMERA_HPP
