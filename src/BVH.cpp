//
// Created by Giovanni Bollati on 10/12/25.
//

#include <BVH.hpp>

#include "glm/common.hpp"
#include "glm/ext/quaternion_geometric.hpp"

#include <cmath>

#include "glm/fwd.hpp"
#include "glm/ext/matrix_integer.hpp"

BVH::BVH(const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, int nPrimitivesPerLeaf)
{
    // init nodes array
    _data = std::vector<BVHNode>();
    // init primitives array
    _primitives = std::vector<uint32_t>();
    _primitives.resize(indices.size() / 3);
    for (int i = 0; i < indices.size() / 3; i++) _primitives[i] = i;

    // init root
    BVHNode root;
    root.aabb.min = glm::vec3(std::numeric_limits<float>::max());
    root.aabb.max = glm::vec3(-std::numeric_limits<float>::max());

    for (int i = 0; i < indices.size(); i+= 3)
    {
        auto t1 = vertices[indices[i]];
        auto t2 = vertices[indices[i + 1]];
        auto t3 = vertices[indices[i + 2]];
        root.aabb.min = glm::min(root.aabb.min, glm::min(t1, glm::min(t2, t3)));
        root.aabb.max = glm::max(root.aabb.max, glm::max(t1, glm::max(t2, t3)));
    }
    _data.push_back(root);
    build(0, 0, static_cast<int>(_primitives.size()), vertices, indices, nPrimitivesPerLeaf);
}

void BVH::build(int nodeID, int start, int count, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, int nPrimitivesPerLeaf)
{
    // SET AABB
    auto min = glm::vec3(std::numeric_limits<float>::max());
    auto max = glm::vec3(-std::numeric_limits<float>::max());

    for (int i = start; i < start + count; i++)
    {
        auto t1 = vertices[indices[_primitives[i] * 3]];
        auto t2 = vertices[indices[_primitives[i] * 3 + 1]];
        auto t3 = vertices[indices[_primitives[i] * 3 + 2]];
        min = glm::min(min, glm::min(t1, glm::min(t2, t3)));
        max = glm::max(max, glm::max(t1, glm::max(t2, t3)));
    }
    _data[nodeID].aabb.min = min;
    _data[nodeID].aabb.max = max;

    if (count < nPrimitivesPerLeaf)
    {
        // set leaf data
        _data[nodeID].leftChild = start;
        _data[nodeID].numPrimitives = count;
        return;
    }

    glm::vec3 extent = _data[nodeID].aabb.max - _data[nodeID].aabb.min;
    int axis = (extent.x > extent.y && extent.x > extent.z) ? 0 : (extent.y > extent.z ? 1 : 2);

    int mid = start + count / 2;
    std::nth_element(_primitives.begin() + start, _primitives.begin() + mid, _primitives.begin() + start + count,
        [&](uint32_t a, uint32_t b) {
            auto getCenter = [&](uint32_t tri) {
                return (vertices[indices[tri * 3]][axis] + vertices[indices[tri * 3 + 1]][axis] + vertices[indices[tri * 3 + 2]][axis]) / 3.0f;
            };
            return getCenter(a) < getCenter(b);
        });

    // create children
    auto leftID = static_cast<int>(_data.size());
    _data.emplace_back();
    _data.emplace_back();

    _data[nodeID].leftChild = leftID;
    _data[nodeID].numPrimitives = 0;
    // recurse on children
    build(leftID, start, mid - start, vertices, indices, nPrimitivesPerLeaf);
    build(leftID + 1, mid, count - (mid - start), vertices, indices, nPrimitivesPerLeaf);
    _data[nodeID].aabb.min = glm::min(_data[leftID].aabb.min, _data[leftID + 1].aabb.min);
    _data[nodeID].aabb.max = glm::max(_data[leftID].aabb.max, _data[leftID + 1].aabb.max);
}

// IEEE 754
bool BVH::rayAABBIntersection(glm::vec3 o, glm::vec3 inverse_d, AABB aabb)
{
    auto tmin = (aabb.min - o) * inverse_d;
    auto tmax = (aabb.max - o) * inverse_d;
    auto tEnter = glm::vec3(fmin(tmin.x, tmax.x), fmin(tmin.y, tmax.y), fmin(tmin.z, tmax.z));
    auto tExit = glm::vec3(fmax(tmin.x, tmax.x), fmax(tmin.y, tmax.y), fmax(tmin.z, tmax.z));

    float t0 = fmax(fmax(tEnter.x, tEnter.y), tEnter.z);
    float t1 = fmin(fmin(tExit.x, tExit.y), tExit.z);
    return t0 <= t1 and t1 >= 0.0f;
}

bool BVH::mollertrumbore(glm::vec3 ray_origin, glm::vec3 ray_dir, glm::vec3 t1, glm::vec3 t2, glm::vec3 t3, float* parameter) {
    const float EPSILON = 1e-7f;
    glm::vec3 edge1 = t2 - t1;
    glm::vec3 edge2 = t3 - t1;

    glm::vec3 h = glm::cross(ray_dir, edge2);
    float a = glm::dot(edge1, h);

    // Se a è vicino a zero, il raggio è parallelo al triangolo
    if (a > -EPSILON && a < EPSILON) return false;

    float f = 1.0f / a;
    glm::vec3 s = ray_origin - t1;
    float u = f * glm::dot(s, h);

    if (u < 0.0f || u > 1.0f) return false;

    glm::vec3 q = glm::cross(s, edge1);
    float v = f * glm::dot(ray_dir, q);

    if (v < 0.0f || u + v > 1.0f) return false;

    // Calcoliamo t per vedere dove l'intersezione si trova sul raggio
    float t = f * glm::dot(edge2, q);

    if (t > EPSILON) {
        *parameter = t;
        return true;
    }

    return false;
}

void BVH::findIntersectingLeaves(int nodeID, const glm::vec3& o, const glm::vec3& inverse_d, std::vector<int>& leaves)
{
    if (rayAABBIntersection(o, inverse_d, _data[nodeID].aabb))
    {
        // if nodeID is not a leaf, insert it and return
        if (_data[nodeID].numPrimitives == 0)
        {
            findIntersectingLeaves(_data[nodeID].leftChild, o, inverse_d, leaves);
            findIntersectingLeaves(_data[nodeID].leftChild + 1, o, inverse_d, leaves);
        }
        else leaves.push_back(nodeID);
    }
}


bool BVH::intersect(glm::vec3 o, glm::vec3 d, const std::vector<glm::vec3>& vertices, const std::vector<uint32_t>& indices, glm::vec3& intersectionPoint)
{
    auto inverse_d = 1.0f / d;
    std::vector<int> leaves;
    std::vector<float> parameters;
    findIntersectingLeaves(0, o, inverse_d, leaves);
    if (leaves.empty()) return false;
    for (auto leaf : leaves)
    {
        for (int i = _data[leaf].leftChild; i < _data[leaf].leftChild + _data[leaf].numPrimitives; i++)
        {
            auto pi = _primitives[i];
            float parameter;
            if (mollertrumbore(o, d, vertices[indices[pi * 3]], vertices[indices[pi * 3 + 1]], vertices[indices[pi * 3 + 2]], &parameter))
            {
                parameters.push_back(parameter);
            }
        }
    }
    if (parameters.size() > 0)
    {
        std::sort(parameters.begin(), parameters.end());
        intersectionPoint = o + d * parameters[0];
        return true;
    }
    return false;
}

std::vector<glm::vec3> BVH::getAABBLines(const AABB& aabb)
{
    std::vector<glm::vec3> lines;
    // XY FACE
    // min -> min +x
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.min.z);
    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.min.z);
    // min -> min +y
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.min.z);
    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.min.z);
    // min +y -> min +x +y
    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.min.z);
    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.min.z);
    // min +x -> min +x +y
    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.min.z);
    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.min.z);

    // XY FACE to XY + z
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.min.z);
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.max.z);

    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.min.z);
    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.max.z);

    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.min.z);
    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.max.z);

    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.min.z);
    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.max.z);

    // XY+Z FACE
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.max.z);
    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.max.z);
    // min -> min +y
    lines.emplace_back(aabb.min.x, aabb.min.y, aabb.max.z);
    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.max.z);
    // min +y -> min +x +y
    lines.emplace_back(aabb.min.x, aabb.max.y, aabb.max.z);
    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.max.z);
    // min +x -> min +x +y
    lines.emplace_back(aabb.max.x, aabb.min.y, aabb.max.z);
    lines.emplace_back(aabb.max.x, aabb.max.y, aabb.max.z);

    return lines;
}

std::vector<glm::vec3> BVH::getLines()
{
    std::vector<glm::vec3> lines;
    for (auto & i : _data)
    {
        auto aabbLines = getAABBLines(i.aabb);
        lines.insert(lines.end(), aabbLines.begin(), aabbLines.end());
    }
    return lines;
}





