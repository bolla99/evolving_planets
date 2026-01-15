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
inline constexpr std::size_t BHV_MAX_TRIANGLES_PER_LEAF = 32;

struct BHVNode
{
    glm::vec3 min;
    glm::vec3 max;
    std::vector<uint32_t> triangles;
    std::vector<std::shared_ptr<BHVNode>> children;
};

enum TrianglePosition
{
    INSIDE, OUTSIDE, INTERSECT
};

class BHV
{
public:
    explicit BHV(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices);
    void build(const std::shared_ptr<BHVNode>& node, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& currentFaces);

    [[nodiscard]] std::vector<uint32_t> siblings(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3) const;
    [[nodiscard]] std::vector<uint32_t> siblings(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3, const std::shared_ptr<BHVNode>& node) const;

private:
    std::shared_ptr<BHVNode> _root;
    static TrianglePosition trianglePosition(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3, const glm::vec3& min, const glm::vec3& max) ;
};

#endif //EVOLVING_PLANETS_BHV_HPP