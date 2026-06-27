//
// Created by Giovanni Bollati on 18/07/25.
//

#ifndef GRAVITYADAPTER_HPP
#define GRAVITYADAPTER_HPP

#include <Mesh.hpp>
#include <gravity.hpp>
#include <BVH.hpp>

#include "GPUComputing.hpp"
#include <Texture.hpp>


namespace GravityAdapter
{
    class Util
    {
    public:
        static std::shared_ptr<Core::Texture> getPotential3DTexture(
            const std::shared_ptr<Geometry::Mesh>& mesh,
            glm::vec3 regionMin, float regionEdge, int size, int tubesResolution
            )
        {
            float R = 0.0f;
            std::vector<glm::vec<3, unsigned int>> faces;
            for (int i = 0; i < mesh->getFacesData().size(); i+=3)
            {
                faces.emplace_back(mesh->getFacesData()[i] + 1, mesh->getFacesData()[i+1] + 1, mesh->getFacesData()[i+2] + 1);
            }
            auto tubes = gravity::get_tubes(mesh->getVertices(), faces, 64, &R);

            auto points = std::vector<glm::vec3>();
            points.reserve(size * size * size);
            for (int k = 0; k < size; k++)
            {
                for (int j = 0; j < size; j++)
                {
                    for (int i = 0; i < size; i++)
                    {
                        float x = regionMin.x + (float)i * (regionEdge / (float)size);
                        float y = regionMin.y + (float)j * (regionEdge / (float)size);
                        float z = regionMin.z + (float)k * (regionEdge / (float)size);
                        points.emplace_back(x, y, z);
                    }
                }
            }

            auto potentialValues = GPUComputing::get_potentials_from_tubes_with_integral(
                glm::value_ptr(tubes.front().t1),
                static_cast<int>(tubes.size()),
                glm::value_ptr(points.front()),
                static_cast<int>(points.size()), R, gravity::G);

            auto data = std::vector<uint8_t>();
            data.resize(points.size() * sizeof(float));
            memcpy(data.data(), potentialValues, points.size() * sizeof(float));
            free(potentialValues);
            return std::make_shared<Core::Texture>(data, size, size, Core::R32F, sizeof(float), Core::Potential3D, true, size);

        }

        static std::pair<std::shared_ptr<Core::Texture>, float> getDensity3DTexture(
            const std::shared_ptr<Geometry::Mesh>& mesh,
            glm::vec3 regionMin, float regionEdge, int size, float phi_atmo
            )
        {
            float R = 0.0f;
            std::vector<glm::vec<3, unsigned int>> faces;
            for (int i = 0; i < mesh->getFacesData().size(); i+=3)
            {
                faces.emplace_back(mesh->getFacesData()[i] + 1, mesh->getFacesData()[i+1] + 1, mesh->getFacesData()[i+2] + 1);
            }
            auto tubes = gravity::get_tubes(mesh->getVertices(), faces, 64, &R);

            // build BVH
            auto bvh = BVH(mesh->getVertices(), mesh->getFacesData(), 4);
            auto& bvhNodes = bvh.getData();
            auto bvhTris = bvh.getTriangles();

            auto points = std::vector<glm::vec3>();
            points.reserve(size * size * size);
            for (int k = 0; k < size; k++)
            {
                for (int j = 0; j < size; j++)
                {
                    for (int i = 0; i < size; i++)
                    {
                        float x = regionMin.x + (float)i * (regionEdge / (float)size);
                        float y = regionMin.y + (float)j * (regionEdge / (float)size);
                        float z = regionMin.z + (float)k * (regionEdge / (float)size);
                        points.emplace_back(x, y, z);
                    }
                }
            }

            auto densityValues = GPUComputing::get_density_3d(
                glm::value_ptr(tubes.front().t1),
                static_cast<int>(tubes.size()),
                bvhNodes.data(),
                static_cast<int>(bvhNodes.size()),
                bvhTris.data(),
                static_cast<int>(bvhTris.size()),
                glm::value_ptr(points.front()),
                static_cast<int>(points.size()),
                phi_atmo,
                R,
                gravity::G
            );

            auto data = std::vector<uint8_t>();
            data.resize(points.size() * sizeof(float));
            memcpy(data.data(), densityValues[0], points.size() * sizeof(float));
            auto radius = *densityValues[1];

            free(densityValues[0]);
            free(densityValues[1]);

            return std::make_pair(std::make_shared<Core::Texture>(
                data, size, size,
                Core::R32F, sizeof(float),
                Core::AtmosphereDensity3D, true, size
                ), radius);
        }

        static std::shared_ptr<Core::Texture> getLightTransmittanceTexture(
            const std::shared_ptr<Geometry::Mesh>& mesh,
            glm::vec3 regionMin, float regionEdge, int size, float phi_atmo,
            float nonZeroDensityRadius, glm::vec3 sunDirection
            )
        {
            float R = 0.0f;
            std::vector<glm::vec<3, unsigned int>> faces;
            for (int i = 0; i < mesh->getFacesData().size(); i+=3)
            {
                faces.emplace_back(mesh->getFacesData()[i] + 1, mesh->getFacesData()[i+1] + 1, mesh->getFacesData()[i+2] + 1);
            }
            auto tubes = gravity::get_tubes(mesh->getVertices(), faces, 64, &R);

            // build BVH
            auto bvh = BVH(mesh->getVertices(), mesh->getFacesData(), 4);
            auto& bvhNodes = bvh.getData();
            auto bvhTris = bvh.getTriangles();

            auto points = std::vector<glm::vec3>();
            points.reserve(size * size * size);
            for (int k = 0; k < size; k++)
            {
                for (int j = 0; j < size; j++)
                {
                    for (int i = 0; i < size; i++)
                    {
                        float x = regionMin.x + (float)i * (regionEdge / (float)size);
                        float y = regionMin.y + (float)j * (regionEdge / (float)size);
                        float z = regionMin.z + (float)k * (regionEdge / (float)size);
                        points.emplace_back(x, y, z);
                    }
                }
            }

            auto lightTransmittanceValues = GPUComputing::get_light_transmittance(
                glm::value_ptr(tubes.front().t1),
                static_cast<int>(tubes.size()),
                bvhNodes.data(),
                static_cast<int>(bvhNodes.size()),
                bvhTris.data(),
                static_cast<int>(bvhTris.size()),
                glm::value_ptr(points.front()),
                static_cast<int>(points.size()),
                phi_atmo,
                R,
                gravity::G, nonZeroDensityRadius,
                sunDirection.x, sunDirection.y, sunDirection.z
            );

            auto data = std::vector<uint8_t>();
            data.resize(points.size() * sizeof(float));
            memcpy(data.data(), lightTransmittanceValues, points.size() * sizeof(float));

            free(lightTransmittanceValues);

            return std::make_shared<Core::Texture>(
                data, size, size,
                Core::R32F, sizeof(float),
                Core::LightTransmittance3D, true, size
                );
        }
    };

    class GravityComputer
    {
    public:
        explicit GravityComputer(const Geometry::Mesh& mesh, int tubesResolution = 64);

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
