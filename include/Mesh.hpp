//
// Created by Giovanni Bollati on 11/06/25.
//

#ifndef MESH_HPP
#define MESH_HPP

#include <cstdint>
#include <vector>
#include <Core/VertexAttributeEnums.hpp>

#include "BSpline.hpp"
#include "glm/glm.hpp"
#include "Rendering/PSOConfigs.hpp"

class Planet;

namespace Geometry
{
    class Mesh
    {
    public:
        Mesh(int numVertices, int numFaces,
             const std::vector<Core::VertexAttributeName>& vertexAttributeNames,
             const std::vector<Core::VertexAttributeType>& vertexAttributeTypes,
             const std::vector<std::vector<uint8_t>>& vertexData,
             const std::vector<uint32_t>& faces);

        Mesh(const Mesh& mesh) = delete;
        Mesh& operator=(const Mesh& mesh) = delete;
        Mesh(Mesh&& mesh) = delete;
        Mesh& operator=(Mesh&& mesh) = delete;

        [[nodiscard]] bool HasAttribute(Core::VertexAttributeName attributeName) const;
        [[nodiscard]] Core::VertexAttributeType GetAttributeType(Core::VertexAttributeName attributeName) const;

        // permit copy, not move
        [[nodiscard]] const std::vector<uint8_t>& getAttributeData(Core::VertexAttributeName attributeName) const;
        [[nodiscard]] const std::vector<uint32_t>& getFacesData() const;

        [[nodiscard]] int getNumVertices() const { return _numVertices; }
        [[nodiscard]] int getNumFaces() const { return _numFaces; }

        [[nodiscard]] std::vector<glm::vec3> getVertices() const;
        [[nodiscard]] std::vector<glm::vec3> getTriangles() const;

        [[nodiscard]] std::vector<glm::vec3> getNormals() const;
        [[nodiscard]] std::vector<glm::vec2> getTextureCoordinated() const;

        // Recompute per-vertex normals as the (area-weighted) average of adjacent triangle normals.
        // If the mesh does not have a Normal attribute, this is a no-op.
        void recalculateNormals();

        [[nodiscard]] std::vector<Core::VertexAttributeName> getAttributes() const;


        static std::shared_ptr<Mesh> dummy();

        // FACTORY METHODS
        static std::shared_ptr<Mesh> quad(const glm::vec4& rect, float depth, const glm::vec4& color, float uvWidth, float uvHeight);

        // Create a unit-radius icosphere (radius = 1) with variable subdivision level.
        // subdivisionLevel = 0 returns a plain icosahedron.
        static std::shared_ptr<Mesh> icosphere(
            int subdivisionLevel,
            const glm::vec4& color = noVertexColor(),
            float radius = 1.0f,
            glm::vec3 center = glm::vec3(0.0f),
            bool onlyPosition = false
        );

        static glm::vec4 noVertexColor()
        {
            return {1.0f, 0.0f, 1.0f, 1.0f};
        }

        static std::shared_ptr<Mesh> fromBSpline(
            const BSpline& curve,
            float step,
            const glm::vec4& color
        );
        static std::shared_ptr<Mesh> fromPolygon(
            const std::vector<glm::vec3>& positions,
            const glm::vec4& color = noVertexColor(),
            bool addInnerVertices = true
            );
        static std::shared_ptr<Mesh> fromPlanet(
            const Planet& planet,
            const glm::vec4& color = noVertexColor(),
            float samplingRes = 0.01f,
            bool onlyPosition = false
            );

        // Build a planet mesh using Poisson Disk Sampling for more uniform distribution
        static std::shared_ptr<Mesh> fromPlanetPD(
            const Planet& planet,
            float minDistance,
            const glm::vec4& color = noVertexColor(),
            bool onlyPosition = false,
            int maxAttempts = 30
            );

        // Build a planet mesh starting from an icosphere topology.
        // It first creates a unit-radius icosphere (subdivisionLevel controls resolution)
        // and then remaps each vertex direction to the planet surface by querying the Planet.
        static std::shared_ptr<Mesh> fromIcoPlanet(
            const Planet& planet,
            int subdivisionLevel,
            const glm::vec4& color = noVertexColor(),
            bool onlyPosition = false
        );

        // Same as fromIcoPlanet, but applies a rocky displacement along the planet normal
        // using the same ridgedFBM approach as fromPlanetRockyfied.
        static std::shared_ptr<Mesh> fromIcoPlanetRockyfied(
            const Planet& planet,
            int subdivisionLevel,
            const glm::vec4& color = noVertexColor(),
            bool onlyPosition = false,
            int fractalOctaves = 10,
            float fractalIntensity = 1.0f,
            float fractalScale = 1.0f
        );

        // Build a planet mesh starting from a cubesphere topology.
        // It creates a subdivided cube projected onto a sphere, then remaps to planet surface.
        static std::shared_ptr<Mesh> fromCubePlanet(
            const Planet& planet,
            int subdivisionLevel,
            const glm::vec4& color = noVertexColor(),
            bool onlyPosition = false
        );

        // Same as fromCubePlanet, but applies rocky displacement.
        static std::shared_ptr<Mesh> fromCubePlanetRockyfied(
            const Planet& planet,
            int subdivisionLevel,
            const glm::vec4& color = noVertexColor(),
            bool onlyPosition = false,
            int fractalOctaves = 10,
            float fractalIntensity = 1.0f,
            float fractalScale = 1.0f
        );

        // noise functions
        static float hash1d(glm::vec3 p) {
            glm::vec3 p3  = glm::fract(p * glm::vec3(.1031, .1030, .0973));
            p3 += dot(p3, glm::vec3(p3.y, p3.z, p3.x) + glm::vec3(33.33));
            return glm::fract((p3.x + p3.y) * p3.z);
        }

        static float smoothNoise(glm::vec3 p)
        {
            glm::vec3 i = floor(p);
            glm::vec3 f = fract(p);
            f = f * f * (glm::vec3(3.0) - f * 2.0f); // Interpolazione Hermite

            // Campioniamo gli 8 angoli del cubo
            float a = hash1d(i + glm::vec3(0,0,0));
            float b = hash1d(i + glm::vec3(1,0,0));
            float c = hash1d(i + glm::vec3(0,1,0));
            float d = hash1d(i + glm::vec3(1,1,0));
            float e = hash1d(i + glm::vec3(0,0,1));
            float g = hash1d(i + glm::vec3(1,0,1));
            float h = hash1d(i + glm::vec3(0,1,1));
            float j = hash1d(i + glm::vec3(1,1,1));

            return glm::mix(glm::mix(glm::mix(a, b, f.x), glm::mix(c, d, f.x), f.y),
            glm::mix(glm::mix(e, g, f.x), glm::mix(h, j, f.x), f.y), f.z);
        }

        static float ridgedFBM(glm::vec3 p, int octaves) {
            float value = 0.0f;
            float amplitude = 0.5f;
            float frequency = 3.0f;  // Increased initial frequency to reduce large features
            float weight = 1.0f;

            for (int i = 0; i < octaves; i++) {
                // Applichiamo la logica RIDGED
                float n = smoothNoise(p * frequency);
                n = 1.0f - std::abs(n); // Qui nasce la cresta rocciosa
                n *= n; // Accentua la cresta

                value += n * amplitude * weight;
                weight = n; // Rende i dettagli dipendenti dalla forma macro (multifrattale)

                frequency *= 2.0f;
                amplitude *= 0.5f;
            }
            return value;
        }

        // Triplanar mapping version of ridgedFBM for more uniform results on spherical surfaces
        static float ridgedFBMTriplanar(glm::vec3 p, glm::vec3 normal, int octaves) {
            // Calculate triplanar blend weights based on normal
            glm::vec3 blendWeights = glm::abs(normal);
            // Sharpen the blending to reduce visible seams
            blendWeights = blendWeights * blendWeights * blendWeights;
            blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);

            // Sample noise from three orthogonal planes
            float noiseX = ridgedFBM(glm::vec3(p.y, p.z, p.x), octaves);
            float noiseY = ridgedFBM(glm::vec3(p.x, p.z, p.y), octaves);
            float noiseZ = ridgedFBM(glm::vec3(p.x, p.y, p.z), octaves);

            // Blend the three samples
            return noiseX * blendWeights.x + noiseY * blendWeights.y + noiseZ * blendWeights.z;
        }

        static std::shared_ptr<Mesh> fromPlanetRockyfied(
            const Planet& planet,
            const glm::vec4& color = noVertexColor(),
            float samplingRes = 0.01f,
            bool onlyPosition = false,
            int fractalOctaves = 10,
            float fractalIntensity = 1.0f
            );
        static std::shared_ptr<Mesh> fromPlanetFitnessColor(
            const Planet& planet,
            const glm::vec4& c1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            const glm::vec4& c2 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            float samplingRes = 0.01f,
            bool discreteColoring = false,
            float fitnessTreshold = 0.9f
            );
        static std::shared_ptr<Mesh> fromPlanetMeanCurvatureColor(
            const Planet& planet,
            const glm::vec4& c1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            const glm::vec4& c2 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            float samplingRes = 0.01f
            );
        static std::shared_ptr<Mesh> fromPlanetGaussCurvatureColor(
            const Planet& planet,
            const glm::vec4& c1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            const glm::vec4& c2 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            float samplingRes = 0.01f
            );
        static std::shared_ptr<Mesh> fromPlanetLaplacianCurvatureColor(
            const Planet& planet,
            const glm::vec4& c1 = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f),
            const glm::vec4& c2 = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f),
            float samplingRes = 0.01f
            );

        // GPU accelerated mesh builder using Metal (metal-cpp)
        static std::shared_ptr<Mesh> fromPlanetGPU(
            const Planet& planet,
            const glm::vec4& color = noVertexColor(),
            float samplingRes = 0.01f,
            bool onlyPosition = false
        );

        static std::shared_ptr<Mesh> aabb(glm::vec3 min, glm::vec3 max, glm::vec4 color);

        // depends on gravity library (ray triangle intersection
        // RAY PICKED DATA
        [[nodiscard]] glm::vec2 uvFromRay(glm::vec3 origin, glm::vec3 direction) const;
        [[nodiscard]] std::pair<bool, glm::vec3> rayIntersection(glm::vec3 origin, glm::vec3 direction) const;

        [[nodiscard]] std::string info() const
        {
            auto info = std::string();
            info += "Mesh with " + std::to_string(_numVertices) + " vertices and " + std::to_string(_numFaces) + " faces\n";
            info += std::string("Normals: ") + (HasAttribute(Core::VertexAttributeName::Normal) ? "yes\n" : "no\n");
            info += std::string("Vertex Color: ") + (HasAttribute(Core::VertexAttributeName::Color) ? "yes\n" : "no\n");
            info += std::string("Texture Coordinates: ") + (HasAttribute(Core::VertexAttributeName::TexCoord) ? "yes\n" : "no\n");
            return info;
        }

        [[nodiscard]] glm::vec3 boundingSphereCenter() const { return _boundingSphereCenter; }
        [[nodiscard]] float boundingSphereRadius() const { return _boundingSphereRadius; }
        [[nodiscard]] glm::vec4 AABBMin() const { return {_AABBMin, 1.0f}; }
        [[nodiscard]] glm::vec4 AABBMax() const { return {_AABBMax, 1.0f}; }
        [[nodiscard]] glm::vec3 centerOfMass() const { return _centerOfMass; }

    private:
        const int _numVertices;
        const int _numFaces;
        const std::vector<Core::VertexAttributeName> _vertexAttributeNames;
        const std::vector<Core::VertexAttributeType> _vertexAttributeTypes;
        std::vector<std::vector<uint8_t>> _vertexData;
        const std::vector<uint32_t> _faces;

        glm::vec3 _AABBMin;
        glm::vec3 _AABBMax;
        glm::vec3 _boundingSphereCenter;
        float _boundingSphereRadius;
        glm::vec3 _centerOfMass;

        [[nodiscard]] glm::vec4 _computeBoundingSphere() const;
        [[nodiscard]] glm::vec3 _computeAABBMin() const;
        [[nodiscard]] glm::vec3 _computeAABBMax() const;
        [[nodiscard]] glm::vec3 _computeCenterOfMass() const;
    };
}

#endif //MESH_HPP
