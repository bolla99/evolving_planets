//
// Created by Giovanni Bollati on 18/07/25.
//

#include <GravityAdapter.hpp>

#include "GPUComputing.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace GravityAdapter
{
    GravityComputer::GravityComputer(const Mesh& mesh, int tubesResolution) : G(9.81f)
    {
        float tubesR;
        auto vertices = mesh.getAttributeData(Core::Position);
        size_t n = vertices.size() / sizeof(glm::vec3);
        auto verticesV3 = std::vector(
            reinterpret_cast<const glm::vec3*>(vertices.data()),
            reinterpret_cast<const glm::vec3*>(vertices.data()) + n);
        auto faces = mesh.getFacesData();
        n = faces.size() / sizeof(glm::vec<3, unsigned int>);
        auto facesV3 = std::vector(
            reinterpret_cast<const glm::vec<3, unsigned int>*>(faces.data()),
            reinterpret_cast<const glm::vec<3, unsigned int>*>(faces.data()) + n * 4
            );
        for (auto& idx: facesV3)
        {
            idx.x++;
            idx.y++;
            idx.z++;
        }

        _tubes = gravity::get_tubes(
            verticesV3,
            facesV3,
            tubesResolution, &tubesR
            );
        _tubesR = tubesR;
    }

    std::vector<glm::vec3> GravityComputer::getTubes()
    {
        if (_tubesAsVec3Cache.empty())
        {
            _tubesAsVec3Cache.reserve(_tubes.size() * 2);
            for (const auto& tube : _tubes)
            {
                _tubesAsVec3Cache.push_back(tube.t1);
                _tubesAsVec3Cache.push_back(tube.t2);
            }
        }
        return _tubesAsVec3Cache;
    }

    glm::vec3 GravityComputer::getGravityCPU(const glm::vec3& position) const
    {
        return gravity::get_gravity_from_tubes_with_integral(position, _tubes, G, _tubesR);
    }

    glm::vec3 GravityComputer::getGravityGPU(const glm::vec3& position) const
    {
        return gravity::get_gravity_from_tubes_with_integral_with_gpu(position, _tubes, G, _tubesR);
    }
    std::vector<glm::vec3> GravityComputer::getGravitiesGPU(const std::vector<glm::vec3>& positions) const
    {
        auto tubesAsFloatPtr = glm::value_ptr(_tubes.front().t1);
        auto positionsAsFloatPtr = glm::value_ptr(positions.front());
        auto floatValues = GPUComputing::get_gravities_from_tubes_with_integral(
            tubesAsFloatPtr, static_cast<int>(_tubes.size()),
            positionsAsFloatPtr, static_cast<int>(positions.size()), _tubesR, G);
        auto gravityValues = std::vector<glm::vec3>();
        gravityValues.reserve(positions.size());
        for (size_t i = 0; i < positions.size(); ++i)
        {
            gravityValues.emplace_back(
                floatValues[i * 3],
                floatValues[i * 3 + 1],
                floatValues[i * 3 + 2]
            );
        }
        return gravityValues;
    }

    glm::vec3 GravityComputer::massCenter() const {
        glm::vec3 centreOfMass = glm::vec3(0.0f);
        for (int i = 0; i < _tubes.size(); i++) {
            centreOfMass += (_tubes[i].t1 + _tubes[i].t2) / 2.0f;
        }
        centreOfMass /= _tubes.size();
        return centreOfMass;
    }
}
