//
// Created by Giovanni Bollati on 18/07/25.
//

#ifndef GRAVITYADAPTER_HPP
#define GRAVITYADAPTER_HPP

#include <Mesh.hpp>
#include <gravity.hpp>


namespace GravityAdapter
{
    class GravityComputer
    {
    public:
        explicit GravityComputer(const Mesh& mesh, int tubesResolution = 64);

        std::vector<glm::vec3> getTubes();
        
        [[nodiscard]] glm::vec3 massCenter() const;


        [[nodiscard]] glm::vec3 getGravityCPU(const glm::vec3& position) const;
        [[nodiscard]] glm::vec3 getGravityGPU(const glm::vec3& position) const;
        [[nodiscard]] std::vector<glm::vec3> getGravitiesGPU(const std::vector<glm::vec3>& positions) const;
        GravityComputer& setG(const float value) { G = value; return *this; }

        //glm::vec3 getGravityFromOctree(const glm::vec3& position);
        //GravityComputer& setUpOctree();

        // octree parameters setters

    private:
        std::vector<gravity::tube> _tubes;
        std::vector<glm::vec3> _tubesAsVec3Cache;
        float _tubesR;
        float G;
    };
}

#endif //GRAVITYADAPTER_HPP
