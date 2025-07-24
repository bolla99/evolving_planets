//
// Created by Giovanni Bollati on 08/07/25.
//

#ifndef PLANET_HPP
#define PLANET_HPP

#include "BSpline.hpp"
#include "glm/vec3.hpp"
#include <vector>

#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/constants.hpp"

/** Planet class represent a B-spline surface
 * It consists of a collection of B-spline that represent
 * the parallels of the planet; the v knot vector is clamped
 * so that the poles are included; the poles are represented
 * by a single point at the top and bottom of the planet, that is
 * a B-spline where every control point is the same.
 * For granting continuity, the four parallels adjacent to the two poles
 * (two for each) are very close to the poles, and they form
 * a plateau with it; the plateau will be allowed to move
 * only as a single point to prevent discontinuities with the
 * remaining parallels.
 **/
class Planet
{
public:
    Planet(int degreeU, int degreeV, const std::vector<std::vector<glm::vec3>>& parallels) :
        _degreeU(degreeU), _degreeV(degreeV), _parallels(parallels)
    {
        // make parallels periodic
        if (_parallels.empty() || _parallels[0].empty())
        {
            throw std::invalid_argument("Parallels cannot be empty.");
        }
        if (_degreeU < 2 || _degreeV < 2)
        {
            throw std::invalid_argument("Degrees must be at least 2.");
        }
        if (_parallels.size() < _degreeV + 1)
        {
            throw std::invalid_argument("Not enough parallels for the specified degree.");
        }
        if (_parallels[0].size() < _degreeU + 1)
        {
            throw std::invalid_argument("Not enough control points in each parallel for the specified degree.");
        }
        for (auto& parallel : _parallels)
        {
            if (parallel.size() != _parallels[0].size())
            {
                throw std::invalid_argument("All parallels must have the same number of control points.");
            }
        }
        // make parallels periodic
        for (int i = 0; i < _parallels.size(); i++)
        {
            for (int j = 0; j < _degreeU; j++)
            {
                _parallels[i].push_back(_parallels[i][j]);
            }
        }
        // make u knots
        _knotsU = BSpline::generateKnots(
            static_cast<int>(_parallels[0].size()),
            _degreeU, 0
        );
        // make v knots
        _knotsV = BSpline::generateKnots(
            static_cast<int>(_parallels.size()),
            _degreeV, _degreeV
        );
    }

    glm::vec3 evaluate(float u, float v) const
    {
        auto uSpan = BSpline::span(u, _knotsU, _degreeU);
        auto vSpan = BSpline::span(v, _knotsV, _degreeV);

        auto uBasis = BSpline::basis(
            uSpan, u, _knotsU, _degreeU
        );
        auto vBasis = BSpline::basis(
            vSpan, v, _knotsV, _degreeV
        );

        glm::vec3 result = glm::vec3(0.0f);
        for (int i = 0; i < _degreeV + 1; i++)
        {
            for (int j = 0; j < _degreeU + 1; j++)
            {
                result += uBasis[j] * vBasis[i] * _parallels[vSpan - _degreeV + 1 + i][uSpan - _degreeU + 1 + j];
            }
        }
        return result;
    }

    const std::vector<std::vector<glm::vec3>>& parallelsCP()
    {
        return _parallels;
    }
    std::vector<std::vector<glm::vec3>> meridiansCP()
    {
        auto meridians = std::vector<std::vector<glm::vec3>>(_parallels[0].size());
        for (size_t i = 0; i < _parallels[0].size(); i++)
        {
            meridians[i] = std::vector<glm::vec3>(_parallels.size());
            for (size_t j = 0; j < _parallels.size(); j++)
            {
                meridians[i][j] = _parallels[j][i];
            }
        }
        return meridians;
    }
    std::vector<BSpline> parallels()
    {

        auto bsplines = std::vector<BSpline>();
        bsplines.reserve(_parallels.size());
        for (size_t i = 0; i < _parallels.size(); i++)
        {
            bsplines.emplace_back(
                _parallels[i],
                _knotsU,
                _degreeU
            );
        }

        return bsplines;
    }
    std::vector<BSpline> meridians()
    {
        auto meridians = this->meridiansCP();
        auto bsplines = std::vector<BSpline>();
        bsplines.reserve(meridians.size());
        for (size_t i = 0; i < meridians.size(); i++)
        {
            bsplines.emplace_back(
                meridians[i],
                _knotsV,
                _degreeV
            );
        }
        return bsplines;
    }

    std::vector<std::vector<glm::vec3>> trueParallels(float uStep = 0.001f, int nParallels = 120)
    {
        std::vector<std::vector<glm::vec3>> result(nParallels);
        for (int i = 0; i < nParallels; i++)
        {
            float v = static_cast<float>(i) / static_cast<float>(nParallels - 1);
            float u = 0.0f;
            while (u < 1.0f)
            {
                result[i].push_back(evaluate(u, v));
                u += uStep;
            }
        }
        return result;
    }
    std::vector<std::vector<glm::vec3>> trueMeridians(float vStep = 0.001f, int nMeridians = 120)
    {
        std::vector<std::vector<glm::vec3>> result(nMeridians);
        for (int i = 0; i < nMeridians; i++)
        {
            float u = static_cast<float>(i) / static_cast<float>(nMeridians - 1);
            float v = 0.0f;
            while (v < 1.0f)
            {
                result[i].push_back(evaluate(u, v));
                v += vStep;
            }
        }
        return result;
    }


    static std::shared_ptr<Planet> sphere(int nParallels, int nMeridians, float radius = 1.0f)
    {
        std::vector<std::vector<glm::vec3>> parallels(nParallels);

        float theta = 0.0f;
        float deltaTheta = glm::pi<float>() / static_cast<float>(nParallels - 5);

        for (int i = 0; i < nParallels; i++)
        {
            if (i > 2 && i < nParallels - 3) theta += deltaTheta;
            std::cout << "theta: " << theta << std::endl;
            //float theta = static_cast<float>(i) / static_cast<float>(nParallels - 1 ) * glm::pi<float>();
            parallels[i].resize(nMeridians);
            for (int j = 0; j < nMeridians; j++)
            {
                float phi = static_cast<float>(j) / static_cast<float>(nMeridians - 1) * glm::two_pi<float>();
                float heightMultiplier = 0.98f;
                // NORTHERN PLATEAU
                if (i == 0)
                {
                    parallels[i][j] = glm::vec3(0.0f, radius * heightMultiplier, 0.0f); // North pole
                }
                else if (i == 1)
                {
                    parallels[i][j] = glm::vec3(
                        radius * 0.1f * sin(deltaTheta) * cos(phi),
                        radius * heightMultiplier,
                        radius * 0.1f * sin(deltaTheta) * sin(phi));
                }
                else if (i == 2)
                {
                    parallels[i][j] = glm::vec3(
                        radius * 0.2f * sin(deltaTheta) * cos(phi),
                        radius * heightMultiplier,
                        radius * 0.2f * sin(deltaTheta) * sin(phi));
                }
                // SOUTHERN PLATEAU
                else if (i == nParallels - 3)
                {
                    parallels[i][j] = glm::vec3(
                        radius * 0.2f * sin(1.0 - deltaTheta) * cos(phi),
                        -radius * heightMultiplier,
                        radius * 0.2f * sin(1.0) * sin(phi));
                }
                else if (i == nParallels - 2)
                {
                    parallels[i][j] = glm::vec3(
                        radius * 0.1f * sin(1.0 - deltaTheta) * cos(phi),
                        -radius * heightMultiplier,
                        radius * 0.1f * sin(1.0 - deltaTheta) * sin(phi));
                }
                else if (i == nParallels - 1)
                {
                    parallels[i][j] = glm::vec3(0.0f, -radius * heightMultiplier, 0.0f); // South pole
                }
                else
                {
                    parallels[i][j] = glm::vec3(
                        radius * sin(theta) * cos(phi),
                        radius * (heightMultiplier + 0.01f) * cos(theta),
                        radius * sin(theta) * sin(phi)
                    );
                }
            }
            parallels[i].pop_back();
        }
        return std::make_shared<Planet>(3, 3, parallels);
    }

    static std::shared_ptr<Planet> asteroid(int nParallels, int nMeridians, float radius = 1.0f)
    {
        std::srand(static_cast<unsigned int>(std::time(nullptr)));
        std::vector<std::vector<glm::vec3>> parallels(nParallels);

            auto northPoleHeight = 0.7f + 0.6f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
            auto southPoleHeight = 0.7f + 0.6f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        for (int i = 0; i < nParallels; i++)
        {
            float theta = static_cast<float>(i) / (nParallels) * glm::pi<float>();
            parallels[i].resize(nMeridians);
            for (int j = 0; j < nMeridians; j++)
            {
                float phi = static_cast<float>(j) / (nMeridians) * glm::two_pi<float>();
                float randomRadius = radius * (0.3f + 1.0f * static_cast<float>(rand()) / static_cast<float>(RAND_MAX));

                if (i == 0)
                {
                    // North pole
                    parallels[i][j] = glm::vec3(0.0f, radius * northPoleHeight, 0.0f);
                }
                else if (i == 1)
                {
                    // First parallel of plateau (north)
                    parallels[i][j] = glm::vec3(
                        radius * 0.1f * sin(theta) * cos(phi),
                        radius * northPoleHeight,
                        radius * 0.1f * sin(theta) * sin(phi)
                    );
                }
                else if (i == 2)
                {
                    // Second parallel of plateau (north)
                    parallels[i][j] = glm::vec3(
                        radius * 0.2f * sin(theta) * cos(phi),
                        radius * northPoleHeight,
                        radius * 0.2f * sin(theta) * sin(phi)
                    );
                }
                else if (i == nParallels - 3)
                {
                    // Second parallel of plateau (south)
                    parallels[i][j] = glm::vec3(
                        radius * 0.2f * sin(theta) * cos(phi),
                        -radius * southPoleHeight,
                        radius * 0.2f * sin(theta) * sin(phi)
                    );
                }
                else if (i == nParallels - 2)
                {
                    // First parallel of plateau (south)
                    parallels[i][j] = glm::vec3(
                        radius * 0.1f * sin(theta) * cos(phi),
                        -radius * southPoleHeight,
                        radius * 0.1f * sin(theta) * sin(phi)
                    );
                }
                else if (i == nParallels - 1)
                {
                    // South pole
                    parallels[i][j] = glm::vec3(0.0f, -radius * southPoleHeight, 0.0f);
                }
                else
                {
                    // Regular random surface
                    float distanceFromPole = std::min(std::abs(theta), std::abs(theta - glm::pi<float>()));
                    float verticalFactor = glm::smoothstep(0.0f, glm::pi<float>() * 0.3f, distanceFromPole);

                    float adjustedY = randomRadius * cos(theta) * verticalFactor +
                        (1.0f - verticalFactor) * (theta < glm::pi<float>() / 2.0f ? radius * 0.95f : -radius * 0.95f);

                    parallels[i][j] = glm::vec3(
                        randomRadius * sin(theta) * cos(phi),
                        adjustedY,
                        randomRadius * sin(theta) * sin(phi)
                    );
                }
            }
        }
        return std::make_shared<Planet>(3, 3, parallels);
    }

    [[nodiscard]] std::vector<glm::vec3> controlPoints() const
    {
        auto controlPoints = std::vector<glm::vec3>();
        for (const auto& parallel : _parallels)
        {
            controlPoints.insert(controlPoints.end(), parallel.begin(), parallel.end());
        }
        return controlPoints;
    }

private:
    int _degreeU = 0;
    int _degreeV = 0;
    // parallels
    std::vector<std::vector<glm::vec3>> _parallels;
    // knots for periodic bspline parallels
    std::vector<int> _knotsU;
    // knots for clamped bspline meridians
    std::vector<int> _knotsV;
};

#endif //PLANET_HPP
