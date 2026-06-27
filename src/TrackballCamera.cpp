//
// Created by Giovanni Bollati on 25/06/25.
//

#include "TrackballCamera.hpp"
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "util.hpp"
#include "glm/ext/matrix_clip_space.hpp"

TrackballCamera::TrackballCamera() : _h(0.0f), _v(0.0f), _distance(0.0f), _orbit(0.0f, 0.0f, 0.0f), _up({0.0f, 1.0f, 0.0f}) {}

void TrackballCamera::pan(float amount)
{
    if (glm::dot(up(), _up) < 0.0f) amount *= -1.0f;
    _h += amount;
    _h = std::fmod(_h, 360.0f);
}
void TrackballCamera::tilt(float amount)
{
    _v += amount;
    _v = std::fmod(_v, 360.0f);
}
void TrackballCamera::zoom(float amount)
{
    _distance += amount;
}

glm::mat4x4 TrackballCamera::getViewMatrix() const
{
    auto startingPosition = glm::vec3(0.0f, 0.0f, _distance);
    auto translate = glm::translate(glm::mat4(1.0f), startingPosition);
    auto translateToOrbit = glm::translate(glm::mat4(1.0f), _orbit);
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(_v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(_h), glm::vec3(0.0f, 1.0f, 0.0f));
    auto radiansToUpVector = util::rotation_between_vectors(glm::vec3(0.0f, 1.0f, 0.0f), _up);
    auto rotateToUp = glm::toMat4(radiansToUpVector);
    return glm::inverse(translateToOrbit * rotateToUp * rotate_y * rotate_x * translate);
}

glm::vec3 TrackballCamera::getPosition() const
{
    auto startingPosition = glm::vec3(0.0f, 0.0f, _distance);
    auto translate = glm::translate(glm::mat4(1.0f), startingPosition);
    auto translateToOrbit = glm::translate(glm::mat4(1.0f), _orbit);
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(_v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(_h), glm::vec3(0.0f, 1.0f, 0.0f));
    auto matrix = translateToOrbit * rotate_y * rotate_x * translate;
    return {matrix[3]};
}

[[nodiscard]] glm::vec3 TrackballCamera::front() const
{
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(_v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(_h), glm::vec3(0.0f, 1.0f, 0.0f));
    auto radiansToUpVector = util::rotation_between_vectors(glm::vec3(0.0f, 1.0f, 0.0f), _up);
    auto rotateToUp = glm::toMat4(radiansToUpVector);

    glm::mat4 worldMatrix = rotateToUp * rotate_y * rotate_x;
    return glm::normalize(glm::vec3(-worldMatrix[2]));
}
[[nodiscard]] glm::vec3 TrackballCamera::right() const
{
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(_v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(_h), glm::vec3(0.0f, 1.0f, 0.0f));
    auto radiansToUpVector = util::rotation_between_vectors(glm::vec3(0.0f, 1.0f, 0.0f), _up);
    auto rotateToUp = glm::toMat4(radiansToUpVector);

    glm::mat4 worldMatrix = rotateToUp * rotate_y * rotate_x;
    return glm::normalize(glm::vec3(worldMatrix[0]));
}
[[nodiscard]] glm::vec3 TrackballCamera::up() const
{
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(_v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(_h), glm::vec3(0.0f, 1.0f, 0.0f));
    auto radiansToUpVector = util::rotation_between_vectors(glm::vec3(0.0f, 1.0f, 0.0f), _up);
    auto rotateToUp = glm::toMat4(radiansToUpVector);

    glm::mat4 worldMatrix = rotateToUp * rotate_y * rotate_x;
    return glm::normalize(glm::vec3(worldMatrix[1]));
}

void TrackballCamera::alignTo(glm::vec3 direction, glm::vec3 updir)
{
    // compute horizontal angle
    auto horizontalDirection = glm::normalize(glm::vec3(direction.x, 0.0f, direction.z));
    _h = glm::degrees(atan2(horizontalDirection.x, horizontalDirection.z));

    // compute vertical angle
    _v = glm::degrees(atan2(direction.y, glm::length(horizontalDirection)));
}

glm::quat TrackballCamera::getOrientation()
{
    return glm::quatLookAt(front(), up());
}

void TrackballCamera::roll(float amount)
{
    _up = glm::rotate(glm::mat4(1.0f), glm::radians(amount), front()) * glm::vec4(_up, 0.0f);
}