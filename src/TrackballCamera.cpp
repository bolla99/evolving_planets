//
// Created by Giovanni Bollati on 25/06/25.
//

#include "TrackballCamera.hpp"
#include <glm/glm.hpp>
#include "Camera.hpp"
#include "glm/ext/matrix_clip_space.hpp"

TrackballCamera::TrackballCamera() : h(0.0f), v(0.0f), distance(0.0f) {}

void TrackballCamera::pan(float amount) { h += amount; }
void TrackballCamera::tilt(float amount) { v += amount; }
void TrackballCamera::zoom(float amount) { distance += amount; }

glm::mat4x4 TrackballCamera::getViewMatrix() const
{
    auto translate = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, distance));
    auto rotate_x = glm::rotate(glm::mat4(1.0f), glm::radians(v), glm::vec3(1.0f, 0.0f, 0.0f));
    auto rotate_y = glm::rotate(glm::mat4(1.0f), glm::radians(h), glm::vec3(0.0f, 1.0f, 0.0f));
    return glm::inverse(rotate_y * rotate_x * translate);
}