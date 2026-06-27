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

    void pan(float amount) override;
    void tilt(float amount) override;
    void roll(float amount) override;
    void zoom(float amount);

    [[nodiscard]] glm::mat4x4 getViewMatrix() const override;
    [[nodiscard]] glm::vec3 getPosition() const override;
    glm::quat getOrientation() override;

    [[nodiscard]] glm::vec3 front() const override;
    [[nodiscard]] glm::vec3 right() const override;
    [[nodiscard]] glm::vec3 up() const override;

    void setOrbitCenter(glm::vec3 center) { _orbit = center; }
    void setDistance(float distance) { _distance = distance; }

    void alignTo(glm::vec3 direction, glm::vec3 updir) override;

private:
    float _h, _v, _distance;
    glm::vec3 _up;
    glm::vec3 _orbit;
};

#endif //TRACKBALLCAMERA_HPP
