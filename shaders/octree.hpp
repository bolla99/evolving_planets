
 // Created by Giovanni Bollati on 08/06/26.
 //
 /*
 #ifndef EVOLVING_PLANETS_OCTREE_HPP
 #define EVOLVING_PLANETS_OCTREE_HPP
 
 #include <metal_stdlib>
 
 using namespace metal;
 
 struct PotentialOctreeInfo
 {
 float4 minBounds;
 int size;
 float _padding1[3];
 float edge;
 float _padding2[3];
 float multiplier;
 };
 
 inline float3 barycentric_coords(float3 a, float3 b, float3 c, float3 p) {
 float3 v0 = b - a;
 float3 v1 = c - a;
 float3 v2 = p - a;
 float d00 = dot(v0, v0);
 float d01 = dot(v0, v1);
 float d11 = dot(v1, v1);
 float d20 = dot(v2, v0);
 float d21 = dot(v2, v1);
 float denom = d00 * d11 - d01 * d01;
 float v = (d11 * d20 - d01 * d21) / denom;
 float w = (d00 * d21 - d01 * d20) / denom;
 float u = 1.0f - v - w;
 return float3(u, v, w);
 }
 
 inline float interpolate(float3 p, float3 box[8], float values[8]) {
 // Corretta l'interpolazione (trilineare) invece del calcolo baricentrico errato del C++ originale
 float3 min_p = box[0];
 float3 max_p = box[7];
 float3 t = (p - min_p) / (max_p - min_p);
 
 float c00 = mix(values[0], values[1], t.x);
 float c10 = mix(values[2], values[3], t.x);
 float c01 = mix(values[4], values[5], t.x);
 float c11 = mix(values[6], values[7], t.x);
 
 float c0 = mix(c00, c10, t.y);
 float c1 = mix(c01, c11, t.y);
 
 return mix(c0, c1, t.z);
 }
 
 inline void get_box(float3 min_p, float edge, thread float3* box) {
 box[0] = float3(min_p.x, min_p.y, min_p.z);
 box[1] = float3(min_p.x + edge, min_p.y, min_p.z);
 box[2] = float3(min_p.x, min_p.y + edge, min_p.z);
 box[3] = float3(min_p.x + edge, min_p.y + edge, min_p.z);
 box[4] = float3(min_p.x, min_p.y, min_p.z + edge);
 box[5] = float3(min_p.x + edge, min_p.y, min_p.z + edge);
 box[6] = float3(min_p.x, min_p.y + edge, min_p.z + edge);
 box[7] = float3(min_p.x + edge, min_p.y + edge, min_p.z + edge);
 }
 
 inline float get_potential_from_octree(float3 p, constant int* octree, float3 min_p, float edge, thread int* depth)
 {
 float3 current_min = min_p;
 float current_edge = edge;
 
 int i = 0;
 for(int d = 0;; d++) {
 if(octree[i] <= 0) {
 // leaf: from i to i + 8 retreive gravity values and interpolate
 float3 box[8];
 get_box(current_min, current_edge, box);
 float values[8];
 for(int j = 0; j < 8; j++) {
 values[j] = as_type<float>(octree[i+j]);
 }
 *depth = d;
 return interpolate(p, box, values);
 } else {
 int k = 0;
 if(p.x > current_min.x + current_edge/2.f) k += 1;
 if(p.y > current_min.y + current_edge/2.f) k += 2;
 if(p.z > current_min.z + current_edge/2.f) k += 4;
 i = octree[i + k];
 current_edge /= 2.f;
 float3 boxes[8];
 get_box(current_min, current_edge * 2.0f, boxes);
 current_min = boxes[k];
 }
 }
 }
 
 #endif //EVOLVING_PLANETS_OCTREE_HPP


//
// Optimized by Giovanni Bollati & Gemini on 10/06/26.
//
*/

#ifndef EVOLVING_PLANETS_OCTREE_HPP
#define EVOLVING_PLANETS_OCTREE_HPP

#include <metal_stdlib>

using namespace metal;

struct PotentialOctreeInfo
{
    float4 minBounds;
    int size;
    float _padding1[3];
    float edge;
    float _padding2[3];
    float multiplier;
};

// Funzione di interpolazione trilineare ottimizzata (passaggio diretto tramite registri)
inline float interpolate_trilinear(float3 t,
                                   float v0, float v1, float v2, float v3,
                                   float v4, float v5, float v6, float v7)
{
    float c00 = mix(v0, v1, t.x);
    float c10 = mix(v2, v3, t.x);
    float c01 = mix(v4, v5, t.x);
    float c11 = mix(v6, v7, t.x);

    float c0 = mix(c00, c10, t.y);
    float c1 = mix(c01, c11, t.y);

    return mix(c0, c1, t.z);
}

// Modifica la firma per includere le variabili di cache
inline float get_potential_from_octree_cached(
    float3 p, constant int* octree, float3 root_min, float root_edge,
    thread float3& cached_min, thread float& cached_edge, thread float* cached_v)
{
    // 1. CACHE HIT: Siamo ancora nello stesso nodo dello step precedente?
    if (cached_edge > 0.0f && all(p >= cached_min) && all(p < cached_min + cached_edge)) {
        float3 t = (p - cached_min) / cached_edge;
        return interpolate_trilinear(t,
            cached_v[0], cached_v[1], cached_v[2], cached_v[3],
            cached_v[4], cached_v[5], cached_v[6], cached_v[7]);
    }

    // 2. CACHE MISS: Il raggio è uscito dal vecchio cubo. Dobbiamo scendere dall'albero.
    float3 current_min = root_min;
    float current_edge = root_edge;
    int i = 0;

    for (int d = 0; d < 12; ++d) {
        int node_index = octree[i];
        
        if (node_index <= 0) {
            // FOGLIA TROVATA! Salviamo i dati nella cache per i prossimi step.
            cached_min = current_min;
            cached_edge = current_edge;
            cached_v[0] = as_type<float>(octree[i + 0]);
            cached_v[1] = as_type<float>(octree[i + 1]);
            cached_v[2] = as_type<float>(octree[i + 2]);
            cached_v[3] = as_type<float>(octree[i + 3]);
            cached_v[4] = as_type<float>(octree[i + 4]);
            cached_v[5] = as_type<float>(octree[i + 5]);
            cached_v[6] = as_type<float>(octree[i + 6]);
            cached_v[7] = as_type<float>(octree[i + 7]);
            
            float3 t = (p - current_min) / current_edge;
            return interpolate_trilinear(t,
                cached_v[0], cached_v[1], cached_v[2], cached_v[3],
                cached_v[4], cached_v[5], cached_v[6], cached_v[7]);
        } else {
            // Navigazione branchless
            float half_edge = current_edge * 0.5f;
            float3 center = current_min + half_edge;
            float3 cmp = step(center, p);
            int k = int(cmp.x) | (int(cmp.y) << 1) | (int(cmp.z) << 2);
            
            current_min += cmp * half_edge;
            i = octree[i + k];
            current_edge = half_edge;
        }
    }
    
    return 0.0f;
}

#endif // EVOLVING_PLANETS_OCTREE_HPP

