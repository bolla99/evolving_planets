#ifndef BVH_INCLUDE
#define BVH_INCLUDE

#include <metal_stdlib>
#include <metal_raytracing>

using namespace metal;

struct BVHInfo {
    int nodesSize;
    float _padding1[3];
    int primitivesSize;
};

struct Triangle {
    packed_float3 v1;
    packed_float3 v2;
    packed_float3 v3;
};

// 24 byte
struct AABB
{
    packed_float3 min = MAXFLOAT; // 12 byte
    packed_float3 max = -MAXFLOAT; // 12 byte
};

// 32 byte
struct BVHNode
{
    AABB aabb; // 24 byte
    int leftChild = 0; // 4 byte
    int numPrimitives = 0; // 4 byte
};

struct IntersectionResult {
    bool happens;
    float distance;
};

struct BVHIntersection {
    bool happens;
    float t;
    int count;
};

// IEEE 754
inline IntersectionResult rayAABBIntersection(float3 o, float3 inverse_d, AABB aabb)
{
    float3 aabb_min = float3(aabb.min);
    float3 aabb_max = float3(aabb.max);

    float3 tmin = (aabb_min - o) * inverse_d;
    float3 tmax = (aabb_max - o) * inverse_d;
    
    // Operazioni vettoriali atomiche (velocissime in hardware)
    float3 tEnter = min(tmin, tmax);
    float3 tExit = max(tmin, tmax);

    float t0 = max(max(tEnter.x, tEnter.y), tEnter.z);
    float t1 = min(min(tExit.x, tExit.y), tExit.z);
    
    return {t0 <= t1 && t1 >= 0.0f, t0};
}

inline float mt_t(float3 ray_origin, float3 ray_dir, float3 t1, float3 t2, float3 t3) {
    const float EPSILON = 1e-7f;
    float3 edge1 = t2 - t1;
    float3 edge2 = t3 - t1;

    float3 h = cross(ray_dir, edge2);
    float a = dot(edge1, h);

    // Se a è vicino a zero, il raggio è parallelo al triangolo
    if (a > -EPSILON && a < EPSILON) return -1.0f;

    float f = 1.0f / a;
    float3 s = ray_origin - t1;
    float u = f * dot(s, h);

    if (u < 0.0f || u > 1.0f) return -1.0f;

    float3 q = cross(s, edge1);
    float v = f * dot(ray_dir, q);

    if (v < 0.0f || u + v > 1.0f) return -1.0f;

    float t = f * dot(edge2, q);
    if (t > EPSILON) {
        return t;
    }
    return -1.0f;
}

inline bool mt(float3 ray_origin, float3 ray_dir, float3 t1, float3 t2, float3 t3) {
    return mt_t(ray_origin, ray_dir, t1, t2, t3) > 0.0f;
}

struct StackItem {
    int nodeID;
    IntersectionResult ir;
};

constant int STACK_SIZE = 16;
inline bool intersect(
                      float3 o,
                      float3 d,
                      constant BVHNode* data,
                      constant Triangle* primitives
                      )
{
    auto inverse_d = 1.0f / d;
    StackItem stack[STACK_SIZE];
    int stackPtr = 0;
    auto ir = rayAABBIntersection(o, inverse_d, data[0].aabb);
    if (!ir.happens) return false;
    
    stack[stackPtr++] = {0, ir};
    while(stackPtr > 0) {
        if (stackPtr >= STACK_SIZE) break;
        auto item = stack[--stackPtr]; // pop node
        auto currentNode = data[item.nodeID];
        auto leftChild = currentNode.leftChild;
        auto numPrimitives = currentNode.numPrimitives;

        if (currentNode.numPrimitives == 0) {
            auto hitLeft = rayAABBIntersection(o, inverse_d, data[leftChild].aabb);
            auto hitRight = rayAABBIntersection(o, inverse_d, data[leftChild+1].aabb);

            if (hitLeft.happens && hitRight.happens) {
                if (hitRight.distance < hitLeft.distance) {
                    stack[stackPtr++] = {leftChild, hitLeft};
                    stack[stackPtr++] = {leftChild + 1, hitRight};
                } else {
                    stack[stackPtr++] = {leftChild + 1, hitRight};
                    stack[stackPtr++] = {leftChild, hitLeft};
                }
            } else if (hitLeft.happens) {
                stack[stackPtr++] = {leftChild, hitLeft};
            } else if (hitRight.happens) {
                stack[stackPtr++] = {leftChild + 1, hitRight};
            }
        } else {
                // node is a leaf: check intersection with the triangles it contains
            for (int i = leftChild; i < leftChild + numPrimitives; i++) {
                auto tri = primitives[i];
                if (mt(o, d, tri.v1, tri.v2, tri.v3)) {
                    return true;
                }
            }
        }
    }
    return false;
}

inline BVHIntersection intersect(
                      float3 o,
                      float3 d,
                      constant BVHNode* data,
                      constant Triangle* primitives,
                      bool countAll
                      )
{
    BVHIntersection res;
    res.happens = false;
    res.t = 1e30f; // MAXFLOAT
    res.count = 0;

    auto inverse_d = 1.0f / d;
    StackItem stack[STACK_SIZE];
    int stackPtr = 0;
    auto ir = rayAABBIntersection(o, inverse_d, data[0].aabb);
    if (!ir.happens) return res;

    stack[stackPtr++] = {0, ir};
    while(stackPtr > 0) {
        if (stackPtr >= STACK_SIZE) break;
        auto item = stack[--stackPtr];

        // Se non dobbiamo contare tutto e abbiamo già trovato un'intersezione più vicina, possiamo potare
        if (!countAll && res.happens && item.ir.distance > res.t) continue;

        auto currentNode = data[item.nodeID];
        auto leftChild = currentNode.leftChild;
        auto numPrimitives = currentNode.numPrimitives;

        if (numPrimitives == 0) {
            auto hitLeft = rayAABBIntersection(o, inverse_d, data[leftChild].aabb);
            auto hitRight = rayAABBIntersection(o, inverse_d, data[leftChild+1].aabb);

            if (hitLeft.happens && hitRight.happens) {
                if (hitRight.distance < hitLeft.distance) {
                    stack[stackPtr++] = {leftChild, hitLeft};
                    stack[stackPtr++] = {leftChild + 1, hitRight};
                } else {
                    stack[stackPtr++] = {leftChild + 1, hitRight};
                    stack[stackPtr++] = {leftChild, hitLeft};
                }
            } else if (hitLeft.happens) {
                stack[stackPtr++] = {leftChild, hitLeft};
            } else if (hitRight.happens) {
                stack[stackPtr++] = {leftChild + 1, hitRight};
            }
        } else {
            for (int i = leftChild; i < leftChild + numPrimitives; i++) {
                auto tri = primitives[i];
                float t = mt_t(o, d, tri.v1, tri.v2, tri.v3);
                if (t > 0.0f) {
                    res.happens = true;
                    res.count++;
                    if (t < res.t) res.t = t;
                    // Se non dobbiamo contare tutto, potremmo fermarci qui se volessimo solo "any hit",
                    // ma l'utente ha chiesto t, quindi di solito si intende il più vicino.
                }
            }
        }
    }
    return res;
}

#endif
