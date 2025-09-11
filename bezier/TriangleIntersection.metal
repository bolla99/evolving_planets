#include <metal_stdlib>
using namespace metal;

struct Triangle {
    packed_float3 v0;
    packed_float3 v1;
    packed_float3 v2;
};

struct IntersectionResult {
    packed_float3 intersectionPoint;
    bool found;
};

struct Result {
    atomic_uint foundIntersection;
};

// Funzione di confronto tra float3 con tolleranza più stretta
inline bool float3_equal(float3 a, float3 b, float epsilon = 1e-4) {
    return all(fabs(a - b) < float3(epsilon));
}

/*
// Funzione che testa se il segmento [orig, dest] interseca il triangolo (v0, v1, v2)
IntersectionResult segment_triangle_intersect(float3 orig, float3 dest, float3 v0, float3 v1, float3 v2) {
    const float EPSILON = 1e-6;
    float3 dir = dest - orig;
    float seg_len = length(dir);
    if (seg_len < EPSILON) return {float3(0.0f, 0.0f, 0.0f), false};
    //dir = normalize(dir); // Normalizza la direzione del segmento
    float3 edge1 = v1 - v0;
    float3 edge2 = v2 - v0;
    float3 h = cross(dir, edge2);
    float a = dot(edge1, h);
    if (fabs(a) < EPSILON) return {float3(0.0f, 0.0f, 0.0f), false}; // Segmento parallelo o coplanare
    float f = 1.0 / a;
    float3 s = orig - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0) retur n{float3(0.0f, 0.0f, 0.0f), false};
    float3 q = cross(s, edge1);
    float v = f * dot(dir, q);
    if (v < 0.0 || u + v > 1.0) return {float3(0.0f, 0.0f, 0.0f), false};
    float t = f * dot(edge2, q);

    if (t > EPSILON && t < seg_len - EPSILON) {
        float3 intersection = orig + normalize(dir) * t;
        // Controlla se il punto coincide con uno dei vertici del segmento
        if (float3_equal(intersection, orig) || float3_equal(intersection, dest)) return {float3(0.0f, 0.0f, 0.0f), false};
        // Controlla se il punto coincide con uno dei vertici del triangolo
        if (float3_equal(intersection, v0) || float3_equal(intersection, v1) || float3_equal(intersection, v2)) return {float3(0.0f, 0.0f, 0.0f), false};
        return true;
    }
    return false;
}
 */

constant IntersectionResult IntResF = {float3(0.0f, 0.0f, 0.0f), false};

// Funzione che testa se il segmento [orig, dest] interseca il triangolo (v0, v1, v2)
IntersectionResult segment_triangle_intersect_right(float3 orig, float3 dest, float3 t1, float3 t2, float3 t3) {
    //const float EPSILON = 0.0000001;
        float3 e1 = t2 - t1;
        float3 e2 = t3 - t1;
    
        float EPSILON = 1e-6;
    
        float3 ray_dir = normalize(dest - orig);

        float3x3 m = float3x3(-ray_dir.x, -ray_dir.y, -ray_dir.z,
                                e1.x, e1.y, e1.z,
                                e2.x, e2.y, e2.z);

        float d = determinant(m);
        // ray parallel to the triangle
        if(d > -EPSILON && d < EPSILON) return IntResF;

        auto o = orig - t1;

        float u = determinant(float3x3(-ray_dir.x, -ray_dir.y, -ray_dir.z, o.x, o.y, o.z, e2.x, e2.y, e2.z)) / d;
        if(u < 0.0 || u > 1.0) return IntResF;
        float v = determinant(float3x3(-ray_dir.x, -ray_dir.y, -ray_dir.z, e1.x, e1.y, e1.z, o.x, o.y, o.z)) / d;
        if(v < 0.0 || u + v > 1.0) return IntResF;
        float t = determinant(float3x3(o.x, o.y, o.z, e1.x, e1.y, e1.z, e2.x, e2.y, e2.z)) / d;
        if(t <= EPSILON) return IntResF;

    
        if (t < 0.0 || t > length(dest - orig)) return IntResF;
        float3 intersection = orig + ray_dir * t;
        // Controlla se il punto coincide con uno dei vertici del segmento
        if (float3_equal(intersection, orig) || float3_equal(intersection, dest)) return IntResF;
        // Controlla se il punto coincide con uno dei vertici del triang
        if (float3_equal(intersection, t1) || float3_equal(intersection, t2) || float3_equal(intersection, t3)) return IntResF;
        return {intersection, true};
}

IntersectionResult tri_tri_intersect(float3 v0, float3 v1, float3 v2, float3 u0, float3 u1, float3 u2) {
    // Controllo adiacenza: se condividono almeno 2 vertici, sono adiacenti
    int shared = 0;
    float3 vertsA[3] = {v0, v1, v2};
    float3 vertsB[3] = {u0, u1, u2};
    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            if (float3_equal(vertsA[i], vertsB[j])) shared++;
        }
    }
    if (shared >= 2) return IntResF;

    // Testa i 3 lati del primo triangolo contro il secondo
    IntersectionResult res;
    res = segment_triangle_intersect_right(v0, v1, u0, u1, u2);
    if (res.found) return res;
    res = segment_triangle_intersect_right(v1, v2, u0, u1, u2);
    if (res.found) return res;
    res = segment_triangle_intersect_right(v2, v0, u0, u1, u2);
    if (res.found) return res;
    // Testa i 3 lati del secondo triangolo contro il primo
    res = segment_triangle_intersect_right(u0, u1, v0, v1, v2);
    if (res.found) return res;
    res = segment_triangle_intersect_right(u1, u2, v0, v1, v2);
    if (res.found) return res;
    res = segment_triangle_intersect_right(u2, u0, v0, v1, v2);
    if (res.found) return res;
    return IntResF;
}

kernel void triangleIntersectionKernel(
    device const Triangle* triangles [[buffer(0)]],
    device unsigned int* pTriCount [[buffer(1)]],
    device Result* result [[buffer(2)]],
    device float* debugBuffer [[buffer(3)]],
    uint2 gid [[thread_position_in_grid]]) {
    // Early exit: se già trovato, non fare altro
    if (atomic_load_explicit(&result->foundIntersection, memory_order_relaxed))
        return;
    uint i = gid.x;
    uint j = gid.y;
    auto triCount = pTriCount[0];
    if (i >= triCount || j >= triCount || i >= j) return;
    IntersectionResult res = tri_tri_intersect(triangles[i].v0, triangles[i].v1, triangles[i].v2, triangles[j].v0, triangles[j].v1, triangles[j].v2);
    if (res.found) {
        // Solo il primo thread che imposta foundIntersection scrive nel buffer di debug
        uint old = atomic_exchange_explicit(&result->foundIntersection, 1, memory_order_relaxed);
        if (old == 0) {
            debugBuffer[0] = triangles[i].v0.x;
            debugBuffer[1] = triangles[i].v0.y;
            debugBuffer[2] = triangles[i].v0.z;
            debugBuffer[3] = triangles[i].v1.x;
            debugBuffer[4] = triangles[i].v1.y;
            debugBuffer[5] = triangles[i].v1.z;
            debugBuffer[6] = triangles[i].v2.x;
            debugBuffer[7] = triangles[i].v2.y;
            debugBuffer[8] = triangles[i].v2.z;
            debugBuffer[9] = triangles[j].v0.x;
            debugBuffer[10] = triangles[j].v0.y;
            debugBuffer[11] = triangles[j].v0.z;
            debugBuffer[12] = triangles[j].v1.x;
            debugBuffer[13] = triangles[j].v1.y;
            debugBuffer[14] = triangles[j].v1.z;
            debugBuffer[15] = triangles[j].v2.x;
            debugBuffer[16] = triangles[j].v2.y;
            debugBuffer[17] = triangles[j].v2.z;
            debugBuffer[18] = res.intersectionPoint.x;
            debugBuffer[19] = res.intersectionPoint.y;
            debugBuffer[20] = res.intersectionPoint.z;
        }
    }
}
