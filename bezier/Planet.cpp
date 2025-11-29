//
// Created by Giovanni Bollati on 25/07/25.
//
#include "Planet.hpp"

#include <array>
#include <random>
#include "BSpline.hpp"
#include "Mesh.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "glm/gtx/intersect.hpp"
#include "glm/gtx/string_cast.hpp"


Planet::Planet(int degreeU, int degreeV, const std::vector<std::vector<glm::vec3>>& parallels) :
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

glm::vec3 Planet::evaluate(float u, float v) const
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

glm::vec3 Planet::uFirstDerivative(float u, float v) const
{
    auto uSpan = BSpline::span(u, _knotsU, _degreeU);
    auto vSpan = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::d1Basis(
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

glm::vec3 Planet::vFirstDerivative(float u, float v) const
{
    auto uSpan = BSpline::span(u, _knotsU, _degreeU);
    auto vSpan = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::basis(
        uSpan, u, _knotsU, _degreeU
    );
    auto vBasis = BSpline::d1Basis(
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

glm::vec3 Planet::uSecondDerivative(float u, float v) const {

    auto uSpan = BSpline::span(u, _knotsU, _degreeU);
    auto vSpan = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::d2Basis(
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
glm::vec3 Planet::vSecondDerivative(float u, float v) const {

    auto uSpan = BSpline::span(u, _knotsU, _degreeU);
    auto vSpan = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::basis(
        uSpan, u, _knotsU, _degreeU
    );
    auto vBasis = BSpline::d2Basis(
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

[[nodiscard]] glm::vec3 Planet::uvMixedDerivative(float u, float v) const
{
    auto uSpan = BSpline::span(u, _knotsU, _degreeU);
    auto vSpan = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::d1Basis(
        uSpan, u, _knotsU, _degreeU
    );
    auto vBasis = BSpline::d1Basis(
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

glm::vec3 Planet::normal(float u, float v) const
{
    return glm::normalize(glm::cross(uFirstDerivative(u, v), vFirstDerivative(u, v)));
}

const std::vector<std::vector<glm::vec3>>& Planet::parallelsCP() const
{
    return _parallels;
}

std::vector<std::vector<glm::vec3>> Planet::meridiansCP() const
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

std::vector<BSpline> Planet::parallels() const
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
std::vector<BSpline> Planet::meridians() const
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

std::vector<std::vector<glm::vec3>> Planet::trueParallels(float uStep, int nParallels) const
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

std::vector<std::vector<glm::vec3>> Planet::trueMeridians(float vStep, int nMeridians) const
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

std::vector<glm::vec3> Planet::normalSticks(float length, float step) const
{
    std::vector<glm::vec3> normals;
    for (float u = 0.0f; u < 1.0f; u += step)
    {
        for (float v = 0.0001f; v <= 1.0f - 0.0001f; v += step)
        {
            auto n = normal(u, v);
            normals.push_back(evaluate(u, v) + n * length);
            normals.push_back(evaluate(u, v));
        }
    }
    return normals;
}

std::vector<glm::vec3> Planet::positions(float uStep, float vStep) const
{
    std::vector<glm::vec3> result;
    for (float u = 0.0f; u < 1.0f; u += uStep)
    {
        for (float v = 0.0f; v < 1.0f; v += vStep)
        {
            result.push_back(evaluate(u, v));
        }
    }
    return result;
}

std::vector<glm::vec3> Planet::positions(const std::vector<std::pair<float, float>>& pairs) const
{
    std::vector<glm::vec3> result;
    for (const auto& pair : pairs)
    {
        result.push_back(evaluate(pair.first, pair.second));
    }
    return result;
}

std::vector<glm::vec3> Planet::normals(float uStep, float vStep) const
{
    std::vector<glm::vec3> result;
    for (float u = 0.0f; u < 1.0f; u += uStep)
    {
        for (float v = 0.0f; v < 1.0f; v += vStep)
        {
            result.push_back(normal(u, v));
        }
    }
    return result;
}

std::vector<glm::vec3> Planet::normals(const std::vector<std::pair<float, float>>& pairs) const
{
    std::vector<glm::vec3> result;
    for (const auto& pair : pairs)
    {
        result.push_back(normal(pair.first, pair.second));
    }
    return result;
}

float Planet::fitness(float u, float v, const GravityAdapter::GravityComputer& gc) const
{
    //std::cout << "Evaluating fitness at (" << u << ", " << v << ")..." << std::endl;
    auto p = evaluate(u, v);
    //std::cout << "Position: " << p.x << ", " << p.y << ", " << p.z << std::endl;
    auto n = normal(u, v);
    //std::cout << "Normal: " << n.x << ", " << n.y << ", " << n.z << std::endl;
    auto g = gc.getGravityGPU(p);
    //std::cout << "Gravity: " << g.x << ", " << g.y << ", " << g.z << std::endl;
    auto result = glm::dot(-n, glm::normalize(g));
    //std::cout << "Fitness at (" << u << ", " << v << "): " << result << std::endl;
    return result;
}

std::shared_ptr<Planet> Planet::sphere(int nParallels, int nMeridians, float radius)
{
    std::vector<std::vector<glm::vec3>> parallels(nParallels);

    float theta = 0.0f;
    float deltaTheta = glm::pi<float>() / static_cast<float>(nParallels - 5);

    for (int i = 0; i < nParallels; i++)
    {
        if (i > 2 && i < nParallels - 3) theta += deltaTheta;
        //std::cout << "theta: " << theta << std::endl;
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
                radius * heightMultiplier - 0.01f,
                radius * 0.1f * sin(deltaTheta) * sin(phi));
            }
            else if (i == 2)
            {
                parallels[i][j] = glm::vec3(
                radius * 0.2f * sin(deltaTheta) * cos(phi),
                radius * heightMultiplier - 0.02f,
                radius * 0.2f * sin(deltaTheta) * sin(phi));
            }
            // SOUTHERN PLATEAU
            else if (i == nParallels - 3)
            {
                parallels[i][j] = glm::vec3(
                radius * 0.2f * sin(1.0 - deltaTheta) * cos(phi),
                -radius * heightMultiplier,
                radius * 0.2f * sin(1.0 - deltaTheta) * sin(phi));
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

std::shared_ptr<Planet> Planet::empty(int nParallels, int nMeridians)
{
    std::vector<std::vector<glm::vec3>> parallels(nParallels);
    for (int i = 0; i < nParallels; i++)
    {
        parallels[i].resize(nMeridians, glm::vec3(0.0f));
    }
    return std::make_shared<Planet>(3, 3, parallels);
}


std::shared_ptr<Planet> Planet::asteroid(int nParallels, int nMeridians, float radius)
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

std::vector<glm::vec3> Planet::controlPoints() const
{
    auto controlPoints = std::vector<glm::vec3>();
    for (const auto& parallel : _parallels)
    {
        controlPoints.insert(controlPoints.end(), parallel.begin(), parallel.end());
    }
    return controlPoints;
}

float Planet::fitness(int sampleSize, int tubesResolution) const
{
    // get positions
    auto step = 1.0f / static_cast<float>(sampleSize);
    auto pairs = Planet::pairs(step, step);
    auto positions = this->positions(pairs);
    auto normals = this->normals(pairs);
    // compute gravity values
    auto mesh = Mesh::fromPlanet(*this, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), step);
    auto gc = GravityAdapter::GravityComputer(*mesh, tubesResolution);
    auto fitnessValue = 0.0f;
    auto gravities = gc.getGravitiesGPU(positions);
    for (int i = 0; i < positions.size(); i++)
    {
        fitnessValue += glm::dot(-normals[i], glm::normalize(gravities[i]));
    }
    return fitnessValue / static_cast<float>(positions.size());
}

void Planet::recenter()
{
    auto centerOfMass = glm::vec3(0.0f);
    int resolution = 32;
    float step = 1.0f / static_cast<float>(resolution);
    float u = 0.0f;
    float v = 0.0f;
    while (u < 1.0f)
    {
        v = 0.0f;
        while (v < 1.0f)
        {
            centerOfMass += evaluate(u, v);
            v += step;
        }
        u += step;
    }
    centerOfMass /= static_cast<float>(resolution * resolution);
    for (auto& parallel : _parallels)
    {
        for (auto& point : parallel)
        {
            point -= centerOfMass;
        }
    }
}

bool Planet::mutate(float absMinDistance, float absMaxDistance, float autointersectionStep)
{
    //std::cout << "Mutating planet" << std::endl;

    // control points copy
    auto originalParallels = _parallels;
    // random direction
    float theta = static_cast<float>(rand()) / RAND_MAX * 2.0f * 3.14159265358979323846f;
    float phi = static_cast<float>(rand()) / RAND_MAX * 3.14159265358979323846f;
    float x = sin(phi) * cos(theta);
    float y = sin(phi) * sin(theta);
    float z = cos(phi);
    auto direction = glm::normalize(glm::vec3(x, y, z));

    // random length between absMindistance and absMaxDistance
    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_real_distribution<float> distribution(absMinDistance, absMaxDistance);
    float distance = distribution(generator);

    // Random parallel
    std::uniform_int_distribution parallelDist(_degreeV - 1, static_cast<int>(_parallels.size()) - 1 - _degreeV + 1);
    int randParallel = parallelDist(generator);
    //std::cout << "random parallel: " << randParallel << std::endl;

    // random meridian
    std::uniform_int_distribution meridianDist(0, static_cast<int>(_parallels[randParallel].size()) - 1 - _degreeU);
    int randMeridian = meridianDist(generator);

    bool plateau_north_moved = false;
    bool plateau_south_moved = false;

    if (randParallel < 3)
    {
        // reduce distance for plateay
        //distance *= 0.1f;
        // translate the whole plateau
        for (auto& point : _parallels[0])
        {
            point += direction * distance;
        }
        for (auto& point : _parallels[1])
        {
            point += direction * distance;
        }
        for (auto& point : _parallels[2])
        {
            point += direction * distance;
        }
        plateau_south_moved = true;
    }
    else if (randParallel >= static_cast<int>(_parallels.size()) - 3)
    {
        // reduce distance for plateay
        //distance *= 0.1f;
        // translate the whole plateau
        for (auto& point : _parallels[_parallels.size() - 3])
        {
            point += direction * distance;
        }
        for (auto& point : _parallels[_parallels.size() - 2])
        {
            point += direction * distance;
        }
        for (auto& point : _parallels[_parallels.size() - 1])
        {
            point += direction * distance;
        }
        plateau_north_moved = true;
    } else
    {
        // Mutate the point
        _parallels[randParallel][randMeridian] += direction * distance;
    }
    // apply to repeated points for periodicy
    if (randMeridian < 3)
    {
        _parallels[randParallel][_parallels[0].size() - 3 + randMeridian] = _parallels[randParallel][randMeridian];
    }

    // Propagazione gaussiana della mutazione
    float sigma = 1.0f; // puoi regolare la "larghezza" della propagazione
    glm::vec3 selected = _parallels[randParallel][randMeridian];
    int nMeridians = static_cast<int>(_parallels[0].size());
    int nPeriodic = 3; // numero di vertici ripetuti per la periodicità

    std::vector<std::vector<glm::vec3>> delta(_parallels.size(), std::vector<glm::vec3>(nMeridians, glm::vec3(0.0f)));
    for (int p = 0; p < static_cast<int>(_parallels.size()); ++p) {
        for (int m = 0; m < nMeridians - nPeriodic; ++m) { // solo sui vertici originali
            glm::vec3 cp = _parallels[p][m];
            float d2 = glm::distance(selected, cp);
            float weight = exp(-d2 * d2 / (2.0f * sigma * sigma));
            // Se il punto è nel plateau, muovi tutto il plateau
            if (p < 3 && !plateau_south_moved) {
                for (int pn = 0; pn < 3; ++pn)
                    for (auto& pt : _parallels[pn]) pt += direction * distance * weight;
                plateau_south_moved = true;
                break;
            } else if (p >= static_cast<int>(_parallels.size()) - 3 && !plateau_north_moved) {
                for (int ps = static_cast<int>(_parallels.size()) - 3; ps < static_cast<int>(_parallels.size()); ++ps)
                    for (auto& pt : _parallels[ps]) pt += direction * distance * weight;
                plateau_north_moved = true;
                break;
            } else if (p >= 3 && p < static_cast<int>(_parallels.size()) - 3) {
                _parallels[p][m] += direction * distance * weight;
            }
        }
    }

    // Copia la mutazione sui vertici ripetuti per la periodicità
    for (auto & _parallel : _parallels) {
        for (int m = 0; m < nPeriodic; ++m) {
            _parallel[nMeridians - nPeriodic + m] = _parallel[m];
        }
    }
    // Smoothing post-processing sui paralleli vicini ai poli (dopo i plateau)
    int nParallels = static_cast<int>(_parallels.size());

    // Sud (dopo plateau_south)
    if (nParallels > 7) { // sicurezza per pianeti piccoli
        // first south parallel that does not belong to the plateau
        int pSouth = 3;
        std::vector<glm::vec3> smoothedSouth(nMeridians);
        for (int m = 0; m < nMeridians - nPeriodic; ++m) {
            // cross mean
            glm::vec3 left = _parallels[pSouth][(m - 1 + nMeridians - nPeriodic) % (nMeridians - nPeriodic)];
            glm::vec3 right = _parallels[pSouth][(m + 1) % (nMeridians - nPeriodic)];
            glm::vec3 self = _parallels[pSouth][m];
            glm::vec3 up = _parallels[pSouth + 1][m];
            glm::vec3 down = _parallels[pSouth - 1][m];
            smoothedSouth[m] = (self + left + right + up + down) / 5.0f;
        }
        // Copia i valori smussati e aggiorna la periodicità
        for (int m = 0; m < nMeridians - nPeriodic; ++m) {
            _parallels[pSouth][m] = smoothedSouth[m];
        }
        for (int m = 0; m < nPeriodic; ++m) {
            _parallels[pSouth][nMeridians - nPeriodic + m] = _parallels[pSouth][m];
        }
        // Nord (dopo plateau_north)
        int pNorth = nParallels - 4;
        std::vector<glm::vec3> smoothedNorth(nMeridians);
        for (int m = 0; m < nMeridians - nPeriodic; ++m) {
            glm::vec3 left = _parallels[pNorth][(m - 1 + nMeridians - nPeriodic) % (nMeridians - nPeriodic)];
            glm::vec3 right = _parallels[pNorth][(m + 1) % (nMeridians - nPeriodic)];
            glm::vec3 self = _parallels[pNorth][m];
            glm::vec3 up = _parallels[pNorth + 1][m];
            glm::vec3 down = _parallels[pNorth - 1][m];
            smoothedNorth[m] = (self + left + right + up + down) / 5.0f;
        }
        for (int m = 0; m < nMeridians - nPeriodic; ++m) {
            _parallels[pNorth][m] = smoothedNorth[m];
        }
        for (int m = 0; m < nPeriodic; ++m) {
            _parallels[pNorth][nMeridians - nPeriodic + m] = _parallels[pNorth][m];
        }
    }

      ////////////////////////////
     ///// VALIDITY CHECKS //////
    ////////////////////////////

    /*
    // check control points are not too close
    for (int p = 2; p < static_cast<int>(_parallels.size() - 2); ++p) {
        for (int m = 0; m < nMeridians - nPeriodic - 1; ++m) {
            if (glm::length(_parallels[p][m] - _parallels[p][(m + 1) % (nMeridians - nPeriodic)]) < 0.04f) {
                _parallels = originalParallels;
                std::cout << "Mutation discarded: control POINTS TOO CLOSE" << std::endl;
                std::cout << "p: " << p << ", m: " << m << std::endl;
                std::cout << "p1: " << glm::to_string(_parallels[p][m]) << std::endl;
                std::cout << "p2: " << glm::to_string(_parallels[p][(m + 1) % (nMeridians - nPeriodic)]) << std::endl;
                return;
            }
        }
    }
    */


    polesSmoothing();
    resetPeriodicy();
    if (isAutointersecating(autointersectionStep))
    {
        _parallels = originalParallels;
        //std::cout << "Mutation discarded: surface triangle intersection detected." << std::endl;
        return false;
    }
    //std::cout << "Mutation applied successfully." << std::endl;
    return true;
}


#import <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>

bool Planet::isAutointersecating(float step) const {
    // 1. Estrai triangoli campionati
    struct Triangle {
        glm::vec3 v0, v1, v2;
    };
    /*
    std::vector<std::vector<glm::vec3>> grid;
    for (float v = 0.0f; v < 1.0f; v += vStep) {
        std::vector<glm::vec3> row;
        for (float u = 0.0f; u < 1.0f; u += uStep) {
            row.push_back(evaluate(u, v));
        }
        grid.push_back(row);
    }
    int rows = static_cast<int>(grid.size());
    int cols = static_cast<int>(grid[0].size());
    std::vector<Triangle> triangles;
    for (int i = 0; i < rows - 1; ++i) {
        for (int j = 0; j < cols - 1; ++j) {
            glm::vec3 a = grid[i][j];
            glm::vec3 b = grid[i+1][j];
            glm::vec3 c = grid[i][j+1];
            glm::vec3 d = grid[i+1][j+1];
            triangles.push_back({{a.x,a.y,a.z},{b.x,b.y,b.z},{c.x,c.y,c.z}});
            triangles.push_back({{b.x,b.y,b.z},{d.x,d.y,d.z},{c.x,c.y,c.z}});
        }
    }
    NS::UInteger triCount = triangles.size();*/
    auto mesh = Mesh::fromPlanet(*this, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f), step);
    auto triangles = std::vector<Triangle>();
    auto vertices = mesh->getTriangles();
    for (int i = 0; i < vertices.size() / 3; i++)
    {
        triangles.push_back(Triangle{vertices[i*3], vertices[i*3 + 1], vertices[i*3 + 2]});
    }
    auto triCount = triangles.size();

    // 2. Setup Metal
    auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!device) return false;
    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    NS::Error* error = nullptr;
    auto libraryPath = NS::Bundle::mainBundle()->resourcePath()->stringByAppendingString(
                NS::String::string("/BezierShaders.metallib", NS::ASCIIStringEncoding)
            );
    auto library = NS::TransferPtr(device->newLibrary(libraryPath, &error));
    if (!library) return false;
    auto function = NS::TransferPtr(library->newFunction(NS::String::string("triangleIntersectionKernel", NS::ASCIIStringEncoding)));
    if (!function) return false;
    auto commandQueue = NS::TransferPtr(device->newCommandQueue());
    NS::Error* errPSO = nullptr;
    auto pipeline = NS::TransferPtr(device->newComputePipelineState(function.get(), &errPSO));
    if (!pipeline) return false;

    // 3. Crea buffer
    auto triBuffer = NS::TransferPtr(device->newBuffer(triangles.data(), triCount * sizeof(Triangle), MTL::ResourceStorageModeShared));
    auto triCountBuffer = NS::TransferPtr(device->newBuffer(&triCount, sizeof(NS::UInteger), MTL::ResourceStorageModeShared));
    uint32_t found = 0;
    auto resultBuffer = NS::TransferPtr(device->newBuffer(&found, sizeof(uint32_t), MTL::ResourceStorageModeShared));
    auto debug = new float[21];
    for (int i = 0; i < 21; i++)
    {
        debug[i] = 5.0f; // Inizializza il buffer di debug
    }
    auto debugBuffer = NS::TransferPtr(device->newBuffer(debug, 21 * sizeof(float), MTL::ResourceStorageModeShared));

    auto commandBuffer = commandQueue->commandBuffer();
    auto encoder = commandBuffer->computeCommandEncoder();
    encoder->setComputePipelineState(pipeline.get());
    // 4. Imposta buffer e lancia il kernel

    encoder->setBuffer(triBuffer.get(), 0, 0);
    encoder->setBuffer(triCountBuffer.get(), 0, 1);
    encoder->setBuffer(resultBuffer.get(), 0, 2);
    encoder->setBuffer(debugBuffer.get(), 0, 3);

    auto gridSize = MTL::Size::Make(triCount, triCount, 1);
    auto w = pipeline->maxTotalThreadsPerThreadgroup();
    auto threadgroupSize = MTL::Size::Make(8, 8, 1);
    encoder->dispatchThreads(gridSize, threadgroupSize);
    encoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();
    // 5. Leggi risultato
    uint32_t* foundPtr = (uint32_t*)resultBuffer->contents();

    //std::cout << "Metal intersection check: " << *foundPtr << std::endl;

    auto debugContents = (float*)debugBuffer->contents();
    auto intersectedTriangles = std::vector<glm::vec3>();
    for (int i = 0; i < 7; i++)
    {
        intersectedTriangles.push_back(glm::vec3(debugContents[i * 3], debugContents[i * 3 + 1], debugContents[i * 3 + 2]));
    }
    if (*foundPtr != 0)
    {
        /*
        std::cout << "Intersected triangles: "  << std::endl << glm::to_string(intersectedTriangles[0]) << ", "
                  << glm::to_string(intersectedTriangles[1]) << ", "
                  << glm::to_string(intersectedTriangles[2]) << ", " << std::endl << "Second: " << std::endl
                  << glm::to_string(intersectedTriangles[3]) << ", "
                  << glm::to_string(intersectedTriangles[4]) << ", "
                  << glm::to_string(intersectedTriangles[5]) << std::endl << "intersection point: " << std::endl
                  << glm::to_string(intersectedTriangles[6]) << std::endl;
                  */
    }

    delete[] debug;
    return (*foundPtr) != 0;
}

bool Planet::differentialMutate(const Planet& p1, const Planet& p2, float scaleFactor, float autoIntersectionStep)
{
    auto originalParallels = _parallels;

    auto p1CPs = p1.parallelsCP();
    auto p2CPs = p2.parallelsCP();
    auto difference = std::vector<std::vector<glm::vec3>>();
    difference.reserve(p1CPs.size());
    for (size_t i = 0; i < p1CPs.size(); i++)
    {
        difference.push_back(std::vector<glm::vec3>());
        for (size_t j = 0; j < p1CPs[i].size(); j++)
        {
            auto diff = p1CPs[i][j] - p2CPs[i][j];
            difference[i].push_back(scaleFactor * diff);
        }
    }
    // apply difference
    for (size_t i = 0; i < _parallels.size(); i++)
    {
        for (size_t j = 0; j < _parallels[i].size(); j++)
        {
            _parallels[i][j] += difference[i][j];
        }
    }
    polesSmoothing();
    resetPeriodicy();
    // check if the planet is still valid
    if (isAutointersecating(autoIntersectionStep))
    {
        //std::cout << "Differential mutation resulted in an invalid planet, reverting changes." << std::endl;
        //_parallels = p1CPs; // revert to p1 control points
        _parallels = originalParallels;
        return false;
    }
    //std::cout << "Differential mutation applied successfully." << std::endl;
    return true;
}

bool Planet::continuousCrossover(const Planet& other, Planet& child, float crossoverRate, float autointersectionStep) const
{
    for (int i = 0; i < _parallels.size(); i++)
    {
        for (int j = 0; j < _parallels[i].size(); j++)
        {
            float alpha = static_cast<float>(rand()) / RAND_MAX;
            alpha = crossoverRate;
            child._parallels[i][j] = (1.0f - alpha) * _parallels[i][j] + alpha * other._parallels[i][j];
        }
    }
    child.polesSmoothing();
    child.resetPeriodicy();
    if (child.isAutointersecating(autointersectionStep))
    {
        return false;
    }
    return true;
}

bool Planet::uniformCrossover(const Planet& other, Planet& child, float crossoverRate, float autointersectionStep) const
{
    // choose south plateau
    if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
    {
        // pick first south plateau
        child._parallels[0] = _parallels[0];
        child._parallels[1] = _parallels[1];
        child._parallels[2] = _parallels[2];
    }
    else
    {
        child._parallels[0] = other._parallels[0];
        child._parallels[1] = other._parallels[1];
        child._parallels[2] = other._parallels[2];
    }
    // choose south plateau
    if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
    {
        auto n = static_cast<int>(_parallels.size());
        child._parallels[n - 1] = _parallels[n - 1];
        child._parallels[n - 2] = _parallels[n - 2];
        child._parallels[n - 3] = _parallels[n - 3];
    }
    else
    {
        auto n = static_cast<int>(other._parallels.size());
        child._parallels[n - 1] = other._parallels[n - 1];
        child._parallels[n - 2] = other._parallels[n - 2];
        child._parallels[n - 3] = other._parallels[n - 3];
    }
    for (int i = 3; i < _parallels.size() - 3; i++)
    {
        for (int j = 0; j < _parallels[i].size(); j++)
        {
            if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
            {
                child._parallels[i][j] = _parallels[i][j];
            }
            else
            {
                child._parallels[i][j] = other._parallels[i][j];
            }
        }
    }
    child.polesSmoothing();
    child.resetPeriodicy();
    if (child.isAutointersecating(autointersectionStep))
    {
        //std::cout << "crossover went bad" << std::endl;
        return false;
    }
    return true;
}

bool Planet::parallelWiseUniformCrossover(const Planet& other, Planet& child, float crossoverRate, float autointersectionStep) const
{
    // choose south plateau
    if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
    {
        // pick first south plateau
        child._parallels[0] = _parallels[0];
        child._parallels[1] = _parallels[1];
        child._parallels[2] = _parallels[2];
    }
    else
    {
        child._parallels[0] = other._parallels[0];
        child._parallels[1] = other._parallels[1];
        child._parallels[2] = other._parallels[2];
    }
    // choose south plateau
    if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
    {
        auto n = static_cast<int>(_parallels.size());
        child._parallels[n - 1] = _parallels[n - 1];
        child._parallels[n - 2] = _parallels[n - 2];
        child._parallels[n - 3] = _parallels[n - 3];
    }
    else
    {
        auto n = static_cast<int>(other._parallels.size());
        child._parallels[n - 1] = other._parallels[n - 1];
        child._parallels[n - 2] = other._parallels[n - 2];
        child._parallels[n - 3] = other._parallels[n - 3];
    }
    for (int i = 3; i < _parallels.size() - 3; i++)
    {
        if (static_cast<float>(rand()) / RAND_MAX < crossoverRate)
        {
            child._parallels[i] = _parallels[i];
        }
        else
        {
            child._parallels[i] = other._parallels[i];
        }
    }
    child.resetPeriodicy();
    if (child.isAutointersecating(autointersectionStep))
    {
        //std::cout << "crossover went bad" << std::endl;
        return false;
    }
    child.polesSmoothing();
    return true;
}


float Planet::diversity(const Planet& other) const
{
    auto similarity = 0.0f;
    for (int i = 0; i < _parallels.size(); i++)
    {
        for (int j = 0; j < _parallels[i].size(); j++)
        {
            similarity += glm::length(_parallels[i][j] - other._parallels[i][j]);
        }
    }
    return similarity / static_cast<float>(controlPoints().size());
}


void Planet::resetPeriodicy()
{
    for (int i = 0; i < static_cast<int>(_parallels.size()); i++)
    {
        for (int j = 0; j < _degreeU; j++)
        {
            _parallels[i][_parallels[i].size() - _degreeU + j] = _parallels[i][j];
        }
    }
}

void Planet::polesSmoothing()
{
    auto nParallels = static_cast<int>(_parallels.size());
    auto nMeridians = static_cast<int>(_parallels[0].size());
    auto nPeriodic = _degreeU;

    // South
    // first south parallel that does not belong to the plateau
    int pSouth = 3;
    std::vector<glm::vec3> smoothedSouth(nMeridians);
    for (int m = 0; m < nMeridians - nPeriodic; ++m) {
        // cross mean
        glm::vec3 left = _parallels[pSouth][(m - 1 + nMeridians - nPeriodic) % (nMeridians - nPeriodic)];
        glm::vec3 right = _parallels[pSouth][(m + 1) % (nMeridians - nPeriodic)];
        glm::vec3 self = _parallels[pSouth][m];
        glm::vec3 up = _parallels[pSouth + 1][m];
        glm::vec3 down = _parallels[pSouth - 1][m];
        smoothedSouth[m] = (self + left + right + up + down) / 5.0f;
    }
    for (int m = 0; m < nMeridians - nPeriodic; ++m) {
        _parallels[pSouth][m] = smoothedSouth[m];
    }
    for (int m = 0; m < nPeriodic; ++m) {
        _parallels[pSouth][nMeridians - nPeriodic + m] = _parallels[pSouth][m];
    }
    // Nord
    int pNorth = nParallels - 4;
    std::vector<glm::vec3> smoothedNorth(nMeridians);
    for (int m = 0; m < nMeridians - nPeriodic; ++m) {
        glm::vec3 left = _parallels[pNorth][(m - 1 + nMeridians - nPeriodic) % (nMeridians - nPeriodic)];
        glm::vec3 right = _parallels[pNorth][(m + 1) % (nMeridians - nPeriodic)];
        glm::vec3 self = _parallels[pNorth][m];
        glm::vec3 up = _parallels[pNorth + 1][m];
        glm::vec3 down = _parallels[pNorth - 1][m];
        smoothedNorth[m] = (self + left + right + up + down) / 5.0f;
    }
    for (int m = 0; m < nMeridians - nPeriodic; ++m) {
        _parallels[pNorth][m] = smoothedNorth[m];
    }
    for (int m = 0; m < nPeriodic; ++m)
        {
        _parallels[pNorth][nMeridians - nPeriodic + m] = _parallels[pNorth][m];
    }
    resetPeriodicy();
}

void Planet::laplacianSmoothing(float step, float threshold)
{
    for (int i = 0; i < _parallels.size(); i++)
    {
        for (int j = 0; j < _parallels[i].size() - _degreeU; j++)
        {
            glm::vec3 left = _parallels[i][(j - 1 + _parallels[i].size() - _degreeU) % (_parallels[i].size() - _degreeU)];
            glm::vec3 right = _parallels[i][(j + 1) % (_parallels[i].size() - _degreeU)];
            glm::vec3 self = _parallels[i][j];
            glm::vec3 up = (i > 2) ? _parallels[i - 1][j] : self;
            glm::vec3 down = (i < _parallels.size() - 1 - 2) ? _parallels[i + 1][j] : self;
            
            float u = static_cast<float>(i) / static_cast<float>(_parallels.size() - 1);
            float v = static_cast<float>(j) / static_cast<float>(_parallels[i].size() - 1);
            auto selfval = evaluate(u, v);
            auto leftval = evaluate(u - 0.1f, v);
            auto rightval = evaluate(u + 0.1f, v);
            auto upval = evaluate(u, v - 0.1f);
            auto downval = evaluate(u, v + 0.1f);
            auto laplacian = (left + right + up + down) / 4.0f - self;
            auto laplacianval = (leftval + rightval + upval + downval) / 4.0f - selfval;
            // move towards the laplacian direction with a step proportional to the distance
            glm::vec3 average = (self + left + right + up + down) / 5.0f;
            
            auto curvature = glm::length(laplacianval);
            
            if (curvature > threshold)
                _parallels[i][j] = glm::mix(self, average, step);
        }
    }
    resetPeriodicy();
}

void Planet::curvatureBasedSmoothing(float step)
{
    auto deltas = std::vector<std::vector<glm::vec3>>(_parallels.size());
    for (int i = 0; i < _parallels.size(); i++) deltas[i] = std::vector<glm::vec3>(_parallels[i].size());
    
    auto u = 0.0f;
    auto v = 0.0f;
    while (u < 1.0f)
    {
        while (v < 1.0f)
        {
            auto curvature = meanCurvature(u, v);
            auto du = uFirstDerivative(u, v);
            auto duLength =glm::length(glm::normalize(du));
            if (duLength < 0.5f || isnan(duLength)) { v+= step; continue; }
            
            auto dv = vFirstDerivative(u, v);
            auto normal = glm::normalize(glm::cross(du, dv));

            auto uSpan = BSpline::span(u, _knotsU, _degreeU);
            auto vSpan = BSpline::span(v, _knotsV, _degreeV);

            auto uBasis = BSpline::basis(
                uSpan, u, _knotsU, _degreeU
            );
            auto vBasis = BSpline::basis(
                vSpan, v, _knotsV, _degreeV
            );

            for (int i = 0; i < _degreeV + 1; i++)
            {
                for (int j = 0; j < _degreeU + 1; j++)
                {
                    //_parallels[vSpan - _degreeV + 1 + i][uSpan - _degreeU + 1 + j] += -0.0f;
                    auto x = - 0.01f * uBasis[j] * vBasis[i] * curvature * normal;
                    
                    //_parallels[vSpan - _degreeV + 1 + i][uSpan - _degreeU + 1 + j] += x;
                    deltas[vSpan - _degreeV + 1 + i][uSpan - _degreeU + 1 + j] += x;
                    //uBasis[j] * vBasis[i] * _parallels[vSpan - _degreeV + 1 + i][uSpan - _degreeU + 1 + j];
                }
            }
            v += step;
        }
        u += step;
    }
    for (int i = _degreeU; i < _parallels.size() - _degreeU; i++) {
        for (int j = 0; j < _parallels[0].size(); j++) {
            _parallels[i][j] += deltas[i][j];
        }
    }
    resetPeriodicy();
}

float Planet::firstFundamentalForm(float u, float v) const {
    auto Su = uFirstDerivative(u, v);
    auto Sv = vFirstDerivative(u, v);
    auto E = glm::dot(Su, Su);
    auto F = glm::dot(Su, Sv);
    auto G = glm::dot(Sv, Sv);
    return E * G - F * F;
}

float Planet::secondFundamentalForm(float u, float v) const {
    auto Su = uFirstDerivative(u, v);
    auto Sv = vFirstDerivative(u, v);
    auto Suu = uSecondDerivative(u, v);
    auto Svv = vSecondDerivative(u, v);
    auto Suv = uvMixedDerivative(u, v);

    // second fundamenta form
    auto normal = glm::normalize(glm::cross(Su, Sv)); // unit normal
    auto e = glm::dot(normal, Suu);
    auto f = glm::dot(normal, Suv);
    auto g = glm::dot(normal, Svv);
    return e * g - f * f;
}

float Planet::gaussCurvature(float u, float v) const
{
    auto epsilon = 0.0000001f;
    auto firstFF = firstFundamentalForm(u, v);
    auto secondFF = secondFundamentalForm(u, v);
    if (std::abs(firstFF) < epsilon) return 0.0f;
    auto K = secondFF / firstFF;

    return K;
}

[[deprecated]] float Planet::meanCurvature(float u, float v) const {
    auto epsilon = 0.0001f;
    auto Su = uFirstDerivative(u, v);
    if (isnan(glm::length(glm::normalize(Su)))) return 0.0f;
    if (glm::length(Su) < epsilon) return 0.0f;
    auto Sv = vFirstDerivative(u, v);
    if (glm::length(Sv) < epsilon) return 0.0f;
    auto Suu = uSecondDerivative(u, v);
    auto Svv = vSecondDerivative(u, v);
    auto Suv = uvMixedDerivative(u, v);
    auto E = glm::dot(Su, Su);
    if (E < 0.0f) throw std::runtime_error("E negativo");
    auto F = glm::dot(Su, Sv);
    auto G = glm::dot(Sv, Sv);
    if (G < 0.0f) throw std::runtime_error("G negativo");
    auto firstFF = E * G - F * F;
    if (firstFF < epsilon) return 0.0f;
    if (glm::length(glm::cross(Su, Sv)) < epsilon) return 0.0f;
    auto normal = glm::normalize(glm::cross(Su, Sv)); // unit normal
    if (isnan(normal.x)) return 0.0f;
    auto e = glm::dot(normal, Suu);
    auto f = glm::dot(normal, Suv);
    auto g = glm::dot(normal, Svv);
    //auto secondFFM = glm::mat2x2(e, f, f, g);

    auto denom = std::max(std::abs((E*G) - (F*F)), epsilon * E*G);
    if (firstFF < epsilon * (E * G) and 2 * firstFF > -epsilon * (E * G)) return 0.0f;
    auto H = (e * G - 2.0f * f * F + g * E) / (2.0f * glm::sign(firstFF) * denom);
    return std::abs(H);

    /*
    glm::mat2x2 first = {glm::vec2{E, F}, glm::vec2{F, G}};
    glm::mat2x2 second = {glm::vec2{e, f}, glm::vec2{f, g}};
    auto matrixH = glm::inverse(first) * second;
    auto H = 0.5f * (matrixH[0][0] + matrixH[1][1]);
    return H;
     */

    /*
    // ORTHONORMAL BASIS APPROACH
    auto su = glm::length(Su);
    auto e1 = glm::normalize(Su);
    auto a = glm::dot(Sv, e1);
    auto e2 = Sv - a * e1;
    auto b = glm::length(e2);
    if (b < epsilon) return 0.0f;
    //e2 = glm::normalize(e2);
    /*
    auto T = glm::mat2x2(su, 0, a, b);
    auto secondFForth = glm::transpose(glm::inverse(T)) * secondFFM * glm::inverse(T);

    auto H = 0.5f * (e / (su * su) + g / (b * b));
    //H *= densityScaleFactor(u, v);

    return H;
    */
}

[[deprecated]] float Planet::laplacianCurvature(float u, float v) const
{
    auto spanU = BSpline::span(u, _knotsU, _degreeU);
    auto spanV = BSpline::span(v, _knotsV, _degreeV);

    auto uBasis = BSpline::basis(
        spanU, u, _knotsU, _degreeU
    );
    auto vBasis = BSpline::basis(
        spanV, v, _knotsV, _degreeV
    );

    float value = 0.0f;

    for (int i = 1; i < _degreeV; i++)
    {
        for (int j = 1; j < _degreeU; j++)
        {
            auto gi = spanV - _degreeV + 1 + i;
            auto gj = spanU - _degreeU + 1 + j;

            auto laplacian = _parallels[gi][gj];
            laplacian -= _parallels[gi - 1][gj] / 4.0f;
            laplacian -= _parallels[gi + 1][gj] / 4.0f;
            laplacian -= _parallels[gi][gj - 1] / 4.0f;
            laplacian -= _parallels[gi][gj + 1] / 4.0f;
            value += glm::length(laplacian) * uBasis[i] * vBasis[j];
        }
    }
    return value;
}

// bidimensional grid where the diversity value is stored for each pair of planets
std::vector<std::vector<float>> Planet::diversityGrid(const std::vector<std::shared_ptr<Planet>>& planets)
{
    auto grid = std::vector<std::vector<float>>(planets.size());
    for (int i = 0; i < planets.size(); i++) { grid[i] = std::vector<float>(planets.size()); }

    for (int i = 0; i < planets.size(); i++)
    {
        for (int j = 0; j < planets.size(); j++)
        {
            grid[i][j] = planets[i]->diversity(*planets[j]);
        }
    }
    return grid;
}

// this function assumes that each planet has minimum diversity with another planet, i.e.
// has a most similar planet among the others.
// this functions returns, for each planet, the diversity value with respect to the most similar planet;
// however, each pair is registered only once: if A and B are the most similar to each other, for A the diversity with
// B will be returned, while for B, diversity with A will be skipped and it will be retured the diversity
// value to the second most similar planet.
std::vector<float> Planet::minDiversities(const std::vector<std::shared_ptr<Planet>>& planets)
{
    auto grid = diversityGrid(planets);
    auto result = std::vector<std::pair<float, int>>();
    for (int i = 0; i < planets.size(); i++)
    {
        int minIndex = i;
        float min = std::numeric_limits<float>::max();
        for (int j = 0; j < planets.size(); j++)
        {
            if (i == j || (j < i && result[j].second == i))
            {
                continue;
            }
            if (grid[i][j] < min) { min = grid[i][j]; minIndex = j; }
        }
        result.emplace_back(min, minIndex);
    }
    auto mins = std::vector<float>();
    for (auto & i : result) mins.push_back(i.first);
    return mins;
}

/*
glm::vec3 Planet::massCenter(const GravityAdapter::GravityComputer& gc) const {
    // tessellation
    auto mesh = Mesh::fromPlanet(*this);
    auto gc = GravityAdapter::GravityComputer(*mesh, 64);
    auto tubes = gc.getTubes();
    glm::vec3 centreOfMass = glm::vec3(0.0f);
    for (int i = 0; i < tubes.size() / 2.0f; i++) {
        centreOfMass += (tubes[i*2] + tubes[i*2 + 1])/2.0f;
    }
    centreOfMass /= (tubes.size() / 2.0f);
    return centreOfMass;
}
 */

















