//
// Created by Giovanni Bollati on 25/06/25.
//

#ifndef TRACKBALLCAMERA_HPP
#define TRACKBALLCAMERA_HPP
#include "Camera.hpp"
#include "glm/mat4x4.hpp"

class TrackballCamera : public Camera
{
public:
    TrackballCamera();

    void pan(float delta);
    void tilt(float delta);
    void zoom(float delta);

    [[nodiscard]] glm::mat4x4 getViewMatrix() const override;

private:
    float h, v, distance;
};

#endif //TRACKBALLCAMERA_HPP
