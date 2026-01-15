//
// Created by Giovanni Bollati on 10/12/25.
//

#include <BHV.hpp>
#include <simd/common.h>

#include "glm/common.hpp"

BHV::BHV(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices)
{
    auto min = glm::vec3(std::numeric_limits<float>::max());
    auto max = glm::vec3(std::numeric_limits<float>::lowest());

    // handle empty mesh
    if (vertices.empty()) {
        _root = std::make_shared<BHVNode>();
        _root->min = min;
        _root->max = max;
        return;
    }

    for (const auto& vertex: vertices)
    {
        min = glm::vec3(std::min(min.x, vertex.x), std::min(min.y, vertex.y), std::min(min.z, vertex.z));
        max = glm::vec3(std::max(max.x, vertex.x), std::max(max.y, vertex.y), std::max(max.z, vertex.z));
    }
    auto maxSide = glm::max(max.x - min.x, glm::max(max.y - min.y, max.z - min.z));
    auto centre = (min + max) / 2.0f;
    min = centre - glm::vec3(maxSide / 2.0f);
    max = centre + glm::vec3(maxSide / 2.0f);
    _root = std::make_shared<BHVNode>();
    _root->min = min;
    _root->max = max;

    auto allFaces = std::vector<uint32_t>();
    allFaces.resize(indices.size() / 3);
    for (size_t i = 0; i < allFaces.size(); i++) allFaces[i] = i;
    build(_root, vertices, indices, allFaces);
}

void BHV::build(const std::shared_ptr<BHVNode>& node, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, const std::vector<uint32_t>& currentFaces)
{
    // if leaf (triangles less than threshold) then set indices and return
    if (currentFaces.empty()) return; // nothing to do

    if (currentFaces.size() <= BHV_MAX_TRIANGLES_PER_LEAF)
    {
        for (auto& face : currentFaces)
        {
            // basic bound check: skip invalid indices
            node->triangles.push_back(face);
        }
    }
    else // subdivide in eight and keep on
    {
        // create children
        node->children.reserve(8);
        const float half = (node->max.x - node->min.x) / 2.0f;
        for (int i = 0; i < 8; i++)
        {
            // set children min and max
            node->children.push_back(std::make_shared<BHVNode>());
            node->children[i]->min = {node->min.x + half * static_cast<float>(i % 2), node->min.y + half * static_cast<float>((i / 2) % 2), node->min.z + half * static_cast<float>((i / 4) % 2)};
            node->children[i]->max = node->children[i]->min + glm::vec3(half);
        }

        auto childFaces = std::vector<std::vector<uint32_t>>();
        childFaces.resize(8);
        // For each triangle, try to put it in a child; if it doesn't fit any child keep it in the parent
        for (unsigned int currentFace : currentFaces)
        {
            auto t1 = vertices[indices[currentFace * 3]];
            auto t2 = vertices[indices[currentFace * 3 + 1]];
            auto t3 = vertices[indices[currentFace * 3 + 2]];
            for (int j = 0; j < 8; j++)
            {
                if (trianglePosition(t1, t2, t3, node->children[j]->min, node->children[j]->max) == INSIDE)
                {
                    childFaces[j].push_back(currentFace);
                    break;
                }
            }
        }
        for (int i = 0; i < 8; i++) build(node->children[i], vertices, indices, childFaces[i]);
    }
}

TrianglePosition BHV::trianglePosition(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3, const glm::vec3& min, const glm::vec3& max)
{
    // CHECK IT TRIANGLE IS INSIDE
    bool allIn = true;
    for (auto& v : {t1, t2, t3})
    {
        allIn &= (v.x >= min.x and v.x <= max.x and v.y >= min.y and v.y <= max.y and v.z >= min.z and v.z <= max.z);
    }
    if (allIn) return INSIDE;

    // IF NOT INSIDE; MUST DECIDE IF TRIANGLE IS INTERSECTING OR NOT; CHECK CENTROID
    auto centroid = (t1 + t2 + t3) / 3.0f;
    if (centroid.x >= min.x and centroid.x <= max.x and centroid.y >= min.y and centroid.y <= max.y and centroid.z >= min.z and centroid.z <= max.z)
    {
        // IF CENTROID IS INSIDE; CONSIDER THE TRIANGLE TO BE INSIDE
        // THIS GUARNATEES THAT EACH TRIANGLE IS ASSIGNED AT LEAST IN ONE VOLUME
        return INSIDE;
    }
    return OUTSIDE;
}

std::vector<uint32_t> BHV::siblings(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3) const
{
    return siblings(t1, t2, t3, _root);
}

std::vector<uint32_t> BHV::siblings(const glm::vec3& t1, const glm::vec3& t2, const glm::vec3& t3, const std::shared_ptr<BHVNode>& node) const
{
    if (node->children.empty())
        return node->triangles;

    for (const auto& child : node->children)
    {
        if (trianglePosition(t1, t2, t3, child->min, child->max) == INSIDE) return siblings(t1, t2, t3, child);
    }
    return {};
}