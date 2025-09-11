//
// Created by Giovanni Bollati on 08/07/25.
//

#ifndef PLANET_HPP
#define PLANET_HPP

#include "BSpline.hpp"
#include "glm/vec3.hpp"
#include <vector>

#include "GravityAdapter.hpp"
#include "glm/ext/scalar_constants.hpp"
#include "glm/gtc/constants.hpp"
#include "cereal/cereal.hpp"

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

    // copy constructor
    Planet(const Planet& other) = default;

    [[nodiscard]] glm::vec3 evaluate(float u, float v) const;
    [[nodiscard]] glm::vec3 uFirstDerivative(float u, float v) const;
    [[nodiscard]] glm::vec3 vFirstDerivative(float u, float v) const;
    [[nodiscard]] glm::vec3 uSecondDerivative(float u, float v) const;
    [[nodiscard]] glm::vec3 vSecondDerivative(float u, float v) const;
    [[nodiscard]] glm::vec3 uvMixedDerivative(float u, float v) const;
    [[nodiscard]] glm::vec3 normal(float u, float v) const;

    [[nodiscard]] const std::vector<std::vector<glm::vec3>>& parallelsCP() const;

    [[nodiscard]] std::vector<std::vector<glm::vec3>> meridiansCP() const;

    [[nodiscard]] std::vector<BSpline> parallels() const;
    [[nodiscard]] std::vector<BSpline> meridians() const;

    [[nodiscard]] std::vector<std::vector<glm::vec3>> trueParallels(float uStep = 0.001f, int nParallels = 120) const;
    [[nodiscard]] std::vector<std::vector<glm::vec3>> trueMeridians(float vStep = 0.001f, int nMeridians = 120) const;

    static std::vector<std::pair<float, float>> pairs(float uStep = 0.01f, float vStep = 0.01f)
    {
        std::vector<std::pair<float, float>> result;
        for (float u = 0.0f; u < 1.0f; u += uStep)
        {
            for (float v = 0.0f; v < 1.0f; v += vStep)
            {
                result.emplace_back(u, v);
            }
        }
        return result;
    }

    [[nodiscard]] int degreeU() const { return _degreeU; }
    [[nodiscard]] int degreeV() const { return _degreeV; }

    [[nodiscard]] std::vector<glm::vec3> normalSticks(float length, float step = 0.01f) const;
    [[nodiscard]] std::vector<glm::vec3> positions(float uStep = 0.01f, float vStep = 0.01f) const;

    [[nodiscard]] std::vector<glm::vec3> positions(const std::vector<std::pair<float, float>>& pairs) const;
    [[nodiscard]] std::vector<glm::vec3> normals(float uStep = 0.01f, float vStep = 0.01f) const;

    [[nodiscard]] std::vector<glm::vec3> normals(const std::vector<std::pair<float, float>>& pairs) const;

    // fitness in one point
    [[nodiscard]] float fitness(float u, float v, const GravityAdapter::GravityComputer& gc) const;
    // global fitness
    [[nodiscard]] float fitness(int sampleSize = 32, int tubesResolution = 32) const;

    void recenter();

    bool mutate(float absMinDistance, float absMaxDistance, float autointersectionStep = 0.01f);
    bool differentialMutate(const Planet& p1, const Planet& p2, float scaleFactor = 0.1f, float autointersectionStep = 0.01f);

    bool continuousCrossover(const Planet& other, Planet& child, float crossoverRate = 0.5f, float autointersectionStep = 0.01f) const;
    bool uniformCrossover(const Planet& other, Planet& child, float crossoverRate = 0.5f, float autointersectionStep = 0.01f) const;
    bool parallelWiseUniformCrossover(const Planet& other, Planet& child, float crossoverRate = 0.5f, float autointersectionStep = 0.01f) const;

    float diversity(const Planet& other) const;

    static std::shared_ptr<Planet> sphere(int nParallels, int nMeridians, float radius = 1.0f);
    static std::shared_ptr<Planet> empty(int nParallels, int nMeridians);
    static std::shared_ptr<Planet> asteroid(int nParallels, int nMeridians, float radius = 1.0f);

    void resetPeriodicy();
    void polesSmoothing();
    void laplacianSmoothing(float step = 0.01f, float threshold = 0.5f);
    float curvature(float u, float v) const;

    [[nodiscard]] std::vector<glm::vec3> controlPoints() const;
    
    template<class Archive>
    void serialize(Archive & archive)
    {
        archive(_parallels); // serialize things by passing them to the archive
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

    [[nodiscard]] bool isAutointersecating(float step = 0.01f) const;
};

#endif //PLANET_HPP
