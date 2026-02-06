//
// Created by Giovanni Bollati on 10/12/25.
//

#ifndef EVOLVING_PLANETS_BHV_HPP
#define EVOLVING_PLANETS_BHV_HPP
#include "glm/vec3.hpp"

#include <vector>
#include <memory>
#include <cstdint>
#include <limits>
#include <algorithm>

// massima dimensione foglia in numero di triangoli
inline constexpr std::size_t BVH_MAX_TRIANGLES_PER_LEAF = 8;

struct AABB
{
    glm::vec3 min = glm::vec3(std::numeric_limits<float>::max());
    glm::vec3 max = glm::vec3(-std::numeric_limits<float>::max());
};
struct BVHNode
{
    AABB aabb;
    int leftChild = 0;
    int numPrimitives = 0;
};

class BVH
{
public:
    explicit BVH(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, int nPrimitivesPerLeaf = 8);

    bool intersect(glm::vec3 o, glm::vec3 d, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, glm::vec3& intersectionPoint);
    std::vector<glm::vec3> getLines();

private:
    std::vector<BVHNode> _data;
    std::vector<uint32_t> _primitives;

    void build(int nodeID, int start, int count, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, int nPrimitivesPerLeaf = 8);
    void findIntersectingLeaves(int nodeID, const glm::vec3& o, const glm::vec3& inverse_d, std::vector<int>& leaves);

    static std::vector<glm::vec3> getAABBLines(const AABB& aabb);
    static bool rayAABBIntersection(glm::vec3 o, glm::vec3 inverse_d, AABB aabb);
    static bool mollertrumbore(glm::vec3 ray_origin, glm::vec3 ray_dir, glm::vec3 t1, glm::vec3 t2, glm::vec3 t3, float* parameter);

};

#endif //EVOLVING_PLANETS_BHV_HPP