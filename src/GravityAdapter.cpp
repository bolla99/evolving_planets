//
// Created by Giovanni Bollati on 18/07/25.
//

#include <GravityAdapter.hpp>

#include "GPUComputing.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace GravityAdapter
{
    GravityComputer::GravityComputer(const Mesh& mesh, int tubesResolution)
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
}
