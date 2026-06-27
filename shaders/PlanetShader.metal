#include <metal_stdlib>
#include "Lighting.hpp"
#include "util.hpp"
#include "bvh.hpp"
#include "rayleigh.hpp"

using namespace metal;

struct Payload {
    uint subdivisionLevel;
    float3 v0, v1, v2;
    float4 color;
};

struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float3 icoPosition;
    uint subd [[flat]];
    float4 currentClipPosition;
    float4 previousClipPosition;
};

struct CompactPlanetInfo {
    float planetRadius;
    float fractalIntensity;
    float fractalScale;
    int useConstantLOD;
    int constantLOD;
    int octaves;
    float deltaMultiplier;
    float minDelta;
    float maxDelta;
    int useRayTracingShadows;
    int useSkirts;
};


float3 get_full_displaced_pos(float3 p_ico, CompactPlanetInfo info, texture2d<float> planetTex, texture2d<float> normalTex, sampler planetSampler, int octaves) {
    auto uv = fromPosToUV(p_ico);
    auto p = planetTex.sample(planetSampler, uv).xyz;
    auto n = normalTex.sample(planetSampler, uv).xyz;
    p += n * ridgedFBM(p * info.fractalScale, octaves) * info.fractalIntensity;
    return p;
}


uint calculateLOD(float3 v0, float3 v1, float3 v2, float4x4 modelMatrix, float3 cameraPos,
                  float4x4 projectionMatrix,
                  CompactPlanetInfo info,
                  texture2d<float> planetTex,
                  texture2d<float> normalTex,
                  sampler planetSampler, int octaves) {
    v0 = normalize(v0);
    v1 = normalize(v1);
    v2 = normalize(v2);
    auto vCenter = normalize((v0 + v1 + v2) / 3.0);

    auto wCenter = get_full_displaced_pos(vCenter, info, planetTex, normalTex, planetSampler, octaves);
    auto w0 = get_full_displaced_pos(v0, info, planetTex, normalTex, planetSampler, octaves);
    auto w1 = get_full_displaced_pos(v1, info, planetTex, normalTex, planetSampler, octaves);
    auto w2 = get_full_displaced_pos(v2, info, planetTex, normalTex, planetSampler, octaves);


    // 4. Calcola la dimensione reale della faccia nel mondo
    //float worldRadiusSubface = max(max(distance(w0, wCenter), distance(w1, wCenter)), distance(w2, wCenter));

    // 5. Calcola la distanza minima dai vertici per evitare pop-in bruschi
    float dCenter = distance(wCenter, cameraPos);
    float d0 = distance(w0, cameraPos);
    float d1 = distance(w1, cameraPos);
    float d2 = distance(w2, cameraPos);
    
    float minDist = min(dCenter, min(d0, min(d1, d2)));
    float dist = max(minDist, 0.001);

    // 6. Calcolo della dimensione proiettata (Screen Space Error)
    // screenRef è 1.0 / tan(fov/2), derivato dalla matrice di proiezione standard.
    float screenRef = projectionMatrix[1][1];
    float projectedSize = (info.planetRadius / 10.0 * screenRef) / dist;

    float maxLOD = 7.0;
    
    // 7. Mappatura con Curvatura
    // Portiamo il valore in un range normalizzato 0.0 - 1.0 basato sul range desiderato (0-9)
    float baseLod = log2(projectedSize * 32.0);
    float normalizedLod = clamp(baseLod / maxLOD, 0.0, 1.0);

    // Applichiamo una curva di potenza:
    // 1.0 = lineare (come ora)
    // 2.0 = parabola (molto lento all'inizio, esplode alla fine)
    // 1.5 = una via di mezzo corretta
    float curvedLod = pow(normalizedLod, 1.5);

    // Riportiamo nel range 0-9
    float lodWeight = curvedLod * maxLOD;
    
    return uint(clamp(lodWeight, 2.0, maxLOD));
}

constant float occlusionMargin = 0.5f;
constant int MAX_SUBDIVISIONS_FOR_MESHLET = 3;

// --- CONSTANTS ---
constant float3 icosahedronVertices[12] = {
    float3(-0.525731,  0.850651,  0),
    float3( 0.525731,  0.850651,  0),
    float3(-0.525731, -0.850651,  0),
    float3( 0.525731, -0.850651,  0),
    float3( 0, -0.525731,  0.850651),
    float3( 0,  0.525731,  0.850651),
    float3( 0, -0.525731, -0.850651),
    float3( 0,  0.525731, -0.850651),
    float3( 0.850651,  0, -0.525731),
    float3( 0.850651,  0,  0.525731),
    float3(-0.850651,  0, -0.525731),
    float3(-0.850651,  0,  0.525731)
};

constant uint3 icosahedronIndices[20] = {
    uint3(0, 11, 5), uint3(0, 5, 1), uint3(0, 1, 7), uint3(0, 7, 10), uint3(0, 10, 11),
    uint3(1, 5, 9), uint3(5, 11, 4), uint3(11, 10, 2), uint3(10, 7, 6), uint3(7, 1, 8),
    uint3(3, 9, 4), uint3(3, 4, 2), uint3(3, 2, 6), uint3(3, 6, 8), uint3(3, 8, 9),
    uint3(4, 9, 5), uint3(2, 4, 11), uint3(6, 2, 10), uint3(8, 6, 7), uint3(9, 8, 1)
};

// --- OBJECT SHADER ---
[[object]]
void planet_object_shader(
    uint gid [[threadgroup_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]],
    object_data Payload& payload [[payload]],
    mesh_grid_properties grid,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewMatrix [[buffer(26)]],
    constant float4x4& projectionMatrix [[buffer(27)]],
    constant float4& cameraPosition [[buffer(25)]],
    constant CompactPlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    sampler planetSampler [[sampler(0)]]
) {
    if (tid != 0) return;
    if (gid >= 5120) return;

    uint faceIdx = gid / 256;
    uint subIdx = gid % 256;

    uint3 baseIndices = icosahedronIndices[faceIdx];
    float3 v0 = icosahedronVertices[baseIndices.x];
    float3 v1 = icosahedronVertices[baseIndices.y];
    float3 v2 = icosahedronVertices[baseIndices.z];

    // Calcola i vertici della subface di livello 4
    uint row = 0;
    uint temp_sub = subIdx;
    for (uint r = 0; r < 16; ++r) {
        uint tilesInRow = 2 * (16 - r) - 1;
        if (temp_sub < tilesInRow) {
            row = r;
            break;
        }
        temp_sub -= tilesInRow;
    }
    uint col = temp_sub;
    bool downward = (col % 2 != 0);

    float3 corners_b[3];
    float step = 1.0 / 16.0;
    float fRow = (float)row * step;
    float fCol = (float)(col / 2) * step;

    if (!downward) {
        corners_b[0] = float3(1.0 - fRow - fCol, fCol, fRow);
        corners_b[1] = float3(1.0 - fRow - (fCol + step), fCol + step, fRow);
        corners_b[2] = float3(1.0 - (fRow + step) - fCol, fCol, fRow + step);
    } else {
        // Il triangolo invertito riempie lo spazio tra i vertici di quelli non-invertiti
        corners_b[0] = float3(1.0 - fRow - (fCol + step), fCol + step, fRow);        // Top Right
        corners_b[1] = float3(1.0 - (fRow + step) - (fCol + step), fCol + step, fRow + step); // Bottom Right
        corners_b[2] = float3(1.0 - (fRow + step) - fCol, fCol, fRow + step);       // Bottom Left
    }

    float3 sv[3];
    for (int i = 0; i < 3; ++i) {
        sv[i] = normalize(v0 * corners_b[i].x + v1 * corners_b[i].y + v2 * corners_b[i].z);
    }

    // --- VISIBILITY CULLING ---
    float3 p[4];
    for (int i = 0; i < 3; ++i) {
        p[i] = get_full_displaced_pos(sv[i], planetInfo, planetTex, normalTex, planetSampler, 2);
    }
    float3 vCenter = normalize(sv[0] + sv[1] + sv[2]);
    p[3] = get_full_displaced_pos(vCenter, planetInfo, planetTex, normalTex, planetSampler, 2);

    float3 w[4];
    for (int i = 0; i < 4; ++i) {
        w[i] = (modelMatrix * float4(p[i], 1.0)).xyz;
    }
    
    float3 faceNormal = normalize(cross(w[1] - w[0], w[2] - w[0]));
    float3 viewDir = normalize(w[3] - cameraPosition.xyz);

    if (dot(faceNormal, viewDir) > 0.2) return;

    float4 clipPos[4];
    float4x4 vp = projectionMatrix * viewMatrix;
    bool allOutside[6] = {true, true, true, true, true, true};
    float margin = occlusionMargin;

    for (int i = 0; i < 4; ++i) {
        clipPos[i] = vp * modelMatrix * float4(p[i], 1.0);
        float w_margin = clipPos[i].w * (1.0 + margin);
        if (clipPos[i].x >= -w_margin) allOutside[0] = false;
        if (clipPos[i].x <=  w_margin) allOutside[1] = false;
        if (clipPos[i].y >= -w_margin) allOutside[2] = false;
        if (clipPos[i].y <=  w_margin) allOutside[3] = false;
        if (clipPos[i].z >= -w_margin) allOutside[4] = false;
        if (clipPos[i].z <=  w_margin) allOutside[5] = false;
    }
    
    bool isVisible = true;
    for (int j = 0; j < 6; ++j) {
        if (allOutside[j]) {
            isVisible = false;
            break;
        }
    }
    if (!isVisible) return;

    // determine LOD
    uint subd = 0;
    if (planetInfo.useConstantLOD > 0) {
        subd = planetInfo.constantLOD;
    } else {
        subd = calculateLOD(sv[0], sv[1], sv[2], modelMatrix, cameraPosition.xyz, projectionMatrix, planetInfo, planetTex, normalTex, planetSampler, 1);
    }
    
    uint numMeshlets = 1;
    if (subd > MAX_SUBDIVISIONS_FOR_MESHLET) {
        numMeshlets = 1 << (subd - MAX_SUBDIVISIONS_FOR_MESHLET);
        numMeshlets *= numMeshlets;
    }

    payload.subdivisionLevel = subd;
    payload.v0 = sv[0];
    payload.v1 = sv[1];
    payload.v2 = sv[2];
    
    grid.set_threadgroups_per_grid(uint3(numMeshlets, 1, 1));
}

/*
[[mesh]]
void planet_mesh_shader(
    uint tid [[thread_index_in_threadgroup]],
    uint mid [[threadgroup_position_in_grid]],
    const object_data Payload& payload [[payload]],
    mesh<VertexOut, void, 256, 512, topology::triangle> output,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewMatrix [[buffer(26)]],
    constant float4x4& projectionMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant CompactPlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    sampler planetSampler [[sampler(0)]]
) {
    // each meshlet can be subdivided up to 4 times;
    // if the total subdivision level is lower or equal to 4, then a face will launch a single meshlet, which wil subdivided
    // if the total subdivision level is higher than 4, each face will launch many meshlet, each of them subdividing 4 times
    uint subd = payload.subdivisionLevel;
    
    // n : number of segments each edge is subdivided into
    // example: if subd = 1, 1 << 1 -> 2: an edge is subdivided one time into two segments
    // if subd = 2, 1 << 2 -> 4: an edge is subdivided two times into four segments
    uint n = 1 << subd;
    
    // m is the same as n, but while n refers to the initial face from the object shader, m
    // refers to the meshlet: the n segments are real edge (they will form triangles) while each m segment identifies a meshlet
    uint m = 1;
    if (subd > MAX_SUBDIVISIONS_FOR_MESHLET) m = 1 << (subd - MAX_SUBDIVISIONS_FOR_MESHLET);
    // m stands for the number of row of a face
    // 2 * (totalRows - thisRow) - 1 is the number of items per row
    
    // mid -> meshlet identifier
    uint m_row = 0;
    uint temp_mid = mid;
    for (uint r = 0; r < m; ++r) {
        uint tilesInRow = 2 * (m - r) - 1;
        if (temp_mid < tilesInRow) {
            m_row = r;
            break;
        }
        temp_mid -= tilesInRow;
    }
    uint m_col = temp_mid;
    // m_row and m_col are coordinates for this meshlet inside the face
    
    // n_tile -> segments of a single meshlet
    uint n_tile = n / m;
    
    // even column -> upward
    // odd column -> downward
    // here downward means that this meshlet is downward with respect to the face
    bool downward = (m_col % 2 != 0);

    // is an edge is subdivided n times, the number of triangles is n^2
    uint numTrianglesPerMeshlet = n_tile * n_tile;
    // face triangles
    float3 faceV0 = payload.v0;
    float3 faceV1 = payload.v1;
    float3 faceV2 = payload.v2;
    
    bool useSkirts = true;

    // frustum culling
    bool isThisMeshletVisible = true;
    if (tid == 0) {
        // barycentric coordinates for the vertices of this meshlet
        // order is low left, low right, high if not downward, top left, low, top right otherwise
        float3 corners_b[3];
        if (!downward) {
            // Upward meshlet
            corners_b[0] = float3(1.0 - (float)m_row / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)m_row / m);
            corners_b[1] = float3(1.0 - (float)m_row / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)m_row / m);
            corners_b[2] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)(m_row + 1) / m);
        } else {
            // Downward meshlet
            corners_b[0] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)(m_row + 1) / m);
            corners_b[1] = float3(1.0 - (float)m_row / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)m_row / m);
            corners_b[2] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)(m_row + 1) / m);
        }

        float4x4 vp = projectionMatrix * viewMatrix;
        bool allOutside[6] = {true, true, true, true, true, true};
        
        float margin = occlusionMargin;
        for (int i = 0; i < 3; ++i) {
            // get p_ico of this meshlet vertex
            float3 p_ico = normalize(faceV0 * corners_b[i].x + faceV1 * corners_b[i].y + faceV2 * corners_b[i].z);
            // get actual position (bspline + noise)
            auto p_planet = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 2);
            // clip pos
            float4 clipPos = vp * modelMatrix * float4(p_planet, 1.0);
            // perform checks
            float w_margin = clipPos.w * (1.0 + margin);
            if (clipPos.x >= -w_margin) allOutside[0] = false;
            if (clipPos.x <=  w_margin) allOutside[1] = false;
            if (clipPos.y >= -w_margin) allOutside[2] = false;
            if (clipPos.y <=  w_margin) allOutside[3] = false;
            if (clipPos.z >= 0) allOutside[4] = false;
            if (clipPos.z <=  w_margin) allOutside[5] = false;
        }
        
        // alloutside[j] is true if every vertex of this meshlet fails the test
        // if alloutside[j] is true for some j, then the meshlet is culled
        for (int j = 0; j < 6; ++j) {
            if (allOutside[j]) {
                // thread 0 set the visible flag and return
                output.set_primitive_count(0);
                isThisMeshletVisible = false;
                break;
            }
        }
        if (isThisMeshletVisible) {
            output.set_primitive_count(numTrianglesPerMeshlet);
        }
    }
    
    threadgroup bool isCulled = false;
    if (tid == 0) {
        isCulled = !isThisMeshletVisible;
    }
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (isCulled) return;
    
    // these values are constant: each meshlet is always a triangle subdivided 4 times
    // numVertices per meshlet
    uint numVertices = (n_tile + 1) * (n_tile + 2) / 2;

    uint numThreads = 64;
    // inside each meshlet / threadgroup, each thread handles a single vertex
    // Vertex generation for the tile
    for (uint vIdx = tid; vIdx < numVertices; vIdx += numThreads) {
        // coordinates of the vertex relative to the meshlet
        uint i_tile, j_tile;
        uint temp = vIdx;
        uint row_v = 0;
        // get triangle coordinates inside this meshlet
        for (uint r = 0; r <= n_tile; ++r) {
            uint vInRow = n_tile + 1 - r;
            if (temp < vInRow) {
                row_v = r;
                break;
            }
            temp -= vInRow;
        }
        i_tile = row_v;
        j_tile = temp;
        
        // barycentric coordinates
        float w, u_b;
        if (!downward) {
            w = (float)m_row / m + (float)i_tile / n;
            u_b = (float)(m_col / 2) / m + (float)j_tile / n;
        } else {
            w = (float)(m_row + 1) / m - (float)j_tile / n;
            u_b = (float)(m_col / 2 + 1) / m - (float)i_tile / n;
        }
        float v_b = 1.0 - w - u_b;

        auto p_ico = normalize(faceV0 * v_b + faceV1 * u_b + faceV2 * w);
        auto p = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 12);
        
        VertexOut vout;
        float4 worldPos = modelMatrix * float4(p, 1.0);
        vout.position = viewProjectionMatrix * modelMatrix * float4(p, 1.0);
        vout.worldPosition = worldPos;
        vout.icoPosition = p_ico;
        vout.subd = payload.subdivisionLevel;

        output.set_vertex(vIdx, vout);
    }

    // Index generation for the tile
    for (uint tIdx = tid; tIdx < numTrianglesPerMeshlet; tIdx += numThreads) {
        uint r_tile = 0;
        uint temp = tIdx;
        for (uint r = 0; r < n_tile; ++r) {
            uint tInRow = 2 * (n_tile - r) - 1;
            if (temp < tInRow) {
                r_tile = r;
                break;
            }
            temp -= tInRow;
        }
        uint c_tile = temp;

        uint v_start_row = 0;
        for(uint r = 0; r < r_tile; ++r)
            v_start_row += (n_tile + 1 - r);
        uint v_next_row = v_start_row + (n_tile + 1 - r_tile);

        if (c_tile % 2 == 0) {
            uint j = c_tile / 2;
            if (!downward) {
                output.set_index(tIdx * 3 + 0, v_start_row + j);
                output.set_index(tIdx * 3 + 1, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 2, v_next_row + j);
            } else {
                // Downward tile (m_col % 2 != 0)
                // Winding must be CCW.
                output.set_index(tIdx * 3 + 0, v_start_row + j);
                output.set_index(tIdx * 3 + 1, v_next_row + j);
                output.set_index(tIdx * 3 + 2, v_start_row + j + 1);
            }
        } else {
            uint j = c_tile / 2;
            if (!downward) {
                output.set_index(tIdx * 3 + 0, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 1, v_next_row + j + 1);
                output.set_index(tIdx * 3 + 2, v_next_row + j);
            } else {
                // Downward tile (m_col % 2 != 0)
                // Winding must be CCW.
                output.set_index(tIdx * 3 + 0, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 1, v_next_row + j);
                output.set_index(tIdx * 3 + 2, v_next_row + j + 1);
            }
        }
    }
}
 */

[[mesh]]
void planet_mesh_shader(
    uint tid [[thread_index_in_threadgroup]],
    uint mid [[threadgroup_position_in_grid]],
    const object_data Payload& payload [[payload]],
    mesh<VertexOut, void, 256, 512, topology::triangle> output,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(29)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(30)]],
    constant CompactPlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    sampler planetSampler [[sampler(0)]]
) {
    uint subd = payload.subdivisionLevel;
    uint n = 1 << subd;
    
    uint m = 1;
    if (subd > MAX_SUBDIVISIONS_FOR_MESHLET) m = 1 << (subd - MAX_SUBDIVISIONS_FOR_MESHLET);
    
    uint m_row = 0;
    uint temp_mid = mid;
    for (uint r = 0; r < m; ++r) {
        uint tilesInRow = 2 * (m - r) - 1;
        if (temp_mid < tilesInRow) {
            m_row = r;
            break;
        }
        temp_mid -= tilesInRow;
    }
    uint m_col = temp_mid;
    uint n_tile = n / m;
    bool downward = (m_col % 2 != 0);

    uint numTrianglesPerMeshlet = n_tile * n_tile;
    float3 faceV0 = payload.v0;
    float3 faceV1 = payload.v1;
    float3 faceV2 = payload.v2;

    // Variabile condivisa per calcolare l'orientamento corretto della gonna
    threadgroup float3 sharedMeshletCenter;
    threadgroup bool isCulled = false;

    // --- FRUSTUM CULLING ---
    if (tid == 0) {
        float3 corners_b[3];
        if (!downward) {
            corners_b[0] = float3(1.0 - (float)m_row / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)m_row / m);
            corners_b[1] = float3(1.0 - (float)m_row / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)m_row / m);
            corners_b[2] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)(m_row + 1) / m);
        } else {
            corners_b[0] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2) / m, (float)(m_col / 2) / m, (float)(m_row + 1) / m);
            corners_b[1] = float3(1.0 - (float)m_row / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)m_row / m);
            corners_b[2] = float3(1.0 - (float)(m_row + 1) / m - (float)(m_col / 2 + 1) / m, (float)(m_col / 2 + 1) / m, (float)(m_row + 1) / m);
        }

        // Calcoliamo il centro geometrico del meshlet per orientare la gonna verso l'esterno
        float3 c0 = normalize(faceV0 * corners_b[0].x + faceV1 * corners_b[0].y + faceV2 * corners_b[0].z);
        float3 c1 = normalize(faceV0 * corners_b[1].x + faceV1 * corners_b[1].y + faceV2 * corners_b[1].z);
        float3 c2 = normalize(faceV0 * corners_b[2].x + faceV1 * corners_b[2].y + faceV2 * corners_b[2].z);
        sharedMeshletCenter = normalize(c0 + c1 + c2);

        bool allOutside[6] = {true, true, true, true, true, true};
        float margin = occlusionMargin;

        for (int i = 0; i < 3; ++i) {
            float3 p_ico = normalize(faceV0 * corners_b[i].x + faceV1 * corners_b[i].y + faceV2 * corners_b[i].z);
            auto p_planet = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 2);
            float4 clipPos = viewProjectionMatrix * modelMatrix * float4(p_planet, 1.0);
            float w_margin = clipPos.w * (1.0 + margin);
            if (clipPos.x >= -w_margin) allOutside[0] = false;
            if (clipPos.x <=  w_margin) allOutside[1] = false;
            if (clipPos.y >= -w_margin) allOutside[2] = false;
            if (clipPos.y <=  w_margin) allOutside[3] = false;
            if (clipPos.z >= 0)          allOutside[4] = false;
            if (clipPos.z <=  w_margin) allOutside[5] = false;
        }
        
        bool isThisMeshletVisible = true;
        for (int j = 0; j < 6; ++j) {
            if (allOutside[j]) {
                isThisMeshletVisible = false;
                break;
            }
        }
        isCulled = !isThisMeshletVisible;
        if (isCulled) {
            output.set_primitive_count(0);
        }
    }
    
    threadgroup_barrier(mem_flags::mem_threadgroup);
    if (isCulled) return;
    
    uint numVertices = (n_tile + 1) * (n_tile + 2) / 2;
    uint numThreads = 64;

    // --- GENERAZIONE VERTICI (SUPERFICIE + GONNA) ---
    for (uint vIdx = tid; vIdx < numVertices; vIdx += numThreads) {
        uint i_tile, j_tile;
        uint temp = vIdx;
        uint row_v = 0;
        for (uint r = 0; r <= n_tile; ++r) {
            uint vInRow = n_tile + 1 - r;
            if (temp < vInRow) {
                row_v = r;
                break;
            }
            temp -= vInRow;
        }
        i_tile = row_v;
        j_tile = temp;
        
        float w, u_b;
        if (!downward) {
            w = (float)m_row / m + (float)i_tile / n;
            u_b = (float)(m_col / 2) / m + (float)j_tile / n;
        } else {
            w = (float)(m_row + 1) / m - (float)j_tile / n;
            u_b = (float)(m_col / 2 + 1) / m - (float)i_tile / n;
        }
        float v_b = 1.0 - w - u_b;

        auto p_ico = normalize(faceV0 * v_b + faceV1 * u_b + faceV2 * w);
        auto p = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 8);
        
        VertexOut vout;
        float4 worldPos = modelMatrix * float4(p, 1.0);
        auto position = jitteredViewProjectionMatrix * worldPos;
        vout.position = position;
        vout.worldPosition = worldPos;
        vout.icoPosition = p_ico;
        vout.subd = payload.subdivisionLevel;
        vout.currentClipPosition = viewProjectionMatrix * worldPos;
        vout.previousClipPosition = previousViewProjectionMatrix * worldPos;

        // Scriviamo il vertice standard sulla superficie
        output.set_vertex(vIdx, vout);

        // Se siamo sul bordo esterno del meshlet, estrudiamo il vertice verso l'interno
        if (planetInfo.useSkirts) {
            bool isBoundary = (i_tile == 0) || (j_tile == 0) || (i_tile + j_tile == n_tile);
            if (isBoundary) {
                // Profondità della gonna proporzionale al raggio del pianeta (es. 0.8%)
                float skirtDepth = planetInfo.planetRadius * 0.008f;
                auto normal = normalTex.sample(planetSampler, fromPosToUV(p_ico));
                float3 p_skirt = p - normalize(normal.xyz) * skirtDepth;
                VertexOut vout_skirt = vout;
                vout_skirt.position = jitteredViewProjectionMatrix * modelMatrix * float4(p_skirt, 1.0);
                vout_skirt.worldPosition = modelMatrix * float4(p_skirt, 1.0);
                vout_skirt.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(p_skirt, 1.0);
                vout_skirt.previousClipPosition = previousViewProjectionMatrix * modelMatrix * float4(p_skirt, 1.0);
                // Manteniamo le coordinate sferiche originarie per evitare difetti alle normali
                
                // Lo specchiamo in fondo alla memoria del blocco vertici
                output.set_vertex(numVertices + vIdx, vout_skirt);
            }
        }
    }

    // --- GENERAZIONE INDICI (TRIANGOLI SUPERFICIE) ---
    for (uint tIdx = tid; tIdx < numTrianglesPerMeshlet; tIdx += numThreads) {
        uint r_tile = 0;
        uint temp = tIdx;
        for (uint r = 0; r < n_tile; ++r) {
            uint tInRow = 2 * (n_tile - r) - 1;
            if (temp < tInRow) {
                r_tile = r;
                break;
            }
            temp -= tInRow;
        }
        uint c_tile = temp;

        uint v_start_row = r_tile * (n_tile + 1) - (r_tile * (r_tile - 1)) / 2;
        uint v_next_row = (r_tile + 1) * (n_tile + 1) - ((r_tile + 1) * r_tile) / 2;

        if (c_tile % 2 == 0) {
            uint j = c_tile / 2;
            if (!downward) {
                output.set_index(tIdx * 3 + 0, v_start_row + j);
                output.set_index(tIdx * 3 + 1, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 2, v_next_row + j);
            } else {
                output.set_index(tIdx * 3 + 0, v_start_row + j);
                output.set_index(tIdx * 3 + 1, v_next_row + j);
                output.set_index(tIdx * 3 + 2, v_start_row + j + 1);
            }
        } else {
            uint j = c_tile / 2;
            if (!downward) {
                output.set_index(tIdx * 3 + 0, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 1, v_next_row + j + 1);
                output.set_index(tIdx * 3 + 2, v_next_row + j);
            } else {
                output.set_index(tIdx * 3 + 0, v_start_row + j + 1);
                output.set_index(tIdx * 3 + 1, v_next_row + j);
                output.set_index(tIdx * 3 + 2, v_next_row + j + 1);
            }
        }
    }

    // --- GENERAZIONE INDICI (TRIANGOLI GONNA) ---
    if (planetInfo.useSkirts) {
        uint totalSkirtSegments = 3 * n_tile; // 3 bordi esterni del triangolo principale
        
        for (uint sIdx = tid; sIdx < totalSkirtSegments; sIdx += numThreads) {
            uint edgeId = sIdx / n_tile;
            uint segId = sIdx % n_tile;
            
            uint iA = 0, jA = 0, iB = 0, jB = 0;
            
            // Mappiamo i tre lati esterni del meshlet
            if (edgeId == 0) {       // Lato Inferiore
                iA = 0; jA = segId;
                iB = 0; jB = segId + 1;
            } else if (edgeId == 1) { // Lato Sinistro
                iA = segId; jA = 0;
                iB = segId + 1; jB = 0;
            } else {                  // Lato Diagonale
                iA = segId; jA = n_tile - segId;
                iB = segId + 1; jB = n_tile - (segId + 1);
            }
            
            // Formula matematica diretta per convertire (i, j) in indice lineare senza loop
            uint A = iA * (n_tile + 1) - (iA * (iA - 1)) / 2 + jA;
            uint B = iB * (n_tile + 1) - (iB * (iB - 1)) / 2 + jB;
            
            uint A_skirt = numVertices + A;
            uint B_skirt = numVertices + B;
            
            // Ricostruiamo al volo la posizione sferica per capire l'orientamento
            float wA, u_bA;
            if (!downward) {
                wA = (float)m_row / m + (float)iA / n;
                u_bA = (float)(m_col / 2) / m + (float)jA / n;
            } else {
                wA = (float)(m_row + 1) / m - (float)jA / n;
                u_bA = (float)(m_col / 2 + 1) / m - (float)iA / n;
            }
            float3 pA_ico = normalize(faceV0 * (1.0 - wA - u_bA) + faceV1 * u_bA + faceV2 * wA);
            
            float wB, u_bB;
            if (!downward) {
                wB = (float)m_row / m + (float)iB / n;
                u_bB = (float)(m_col / 2) / m + (float)jB / n;
            } else {
                wB = (float)(m_row + 1) / m - (float)jB / n;
                u_bB = (float)(m_col / 2 + 1) / m - (float)iB / n;
            }
            float3 pB_ico = normalize(faceV0 * (1.0 - wB - u_bB) + faceV1 * u_bB + faceV2 * wB);
            
            // Calcoliamo la direzione che punta "fuori" dal perimetro del meshlet
            float3 edgeMidpoint = normalize(pA_ico + pB_ico);
            float3 outwardDir = normalize(edgeMidpoint - sharedMeshletCenter);
            
            // Calcolo del vettore normale del muro della gonna (il muro va da A a B, e scende verso il centro: -pA_ico)
            float3 N_wall = cross(pB_ico - pA_ico, -pA_ico);
            
            // Gli indici della gonna partono subito dopo quelli del meshlet di superficie
            uint tIdx1 = numTrianglesPerMeshlet + sIdx * 2;
            uint tIdx2 = tIdx1 + 1;
            
            // Controllo del Winding Order dinamico: impedisce il Backface Culling errato
            if (dot(N_wall, outwardDir) >= 0.0f) {
                output.set_index(tIdx1 * 3 + 0, A);
                output.set_index(tIdx1 * 3 + 1, B);
                output.set_index(tIdx1 * 3 + 2, A_skirt);
                
                output.set_index(tIdx2 * 3 + 0, B);
                output.set_index(tIdx2 * 3 + 1, B_skirt);
                output.set_index(tIdx2 * 3 + 2, A_skirt);
            } else {
                output.set_index(tIdx1 * 3 + 0, A);
                output.set_index(tIdx1 * 3 + 1, A_skirt);
                output.set_index(tIdx1 * 3 + 2, B);
                
                output.set_index(tIdx2 * 3 + 0, B);
                output.set_index(tIdx2 * 3 + 1, A_skirt);
                output.set_index(tIdx2 * 3 + 2, B_skirt);
            }
        }
    }

    // --- RILASCIO CONTEGGI FINALI ALLA GPU ---
    threadgroup_barrier(mem_flags::mem_none);
    if (tid == 0) {
        uint finalVertices = numVertices;
        uint finalPrimitives = numTrianglesPerMeshlet;
        if (planetInfo.useSkirts) {
            finalVertices = numVertices * 2;
            finalPrimitives = numTrianglesPerMeshlet + (3 * n_tile * 2);
        }
        output.set_primitive_count(finalPrimitives);
    }
}


float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

// --- FRAGMENT SHADER ---
[[fragment]]
FragmentOut planet_fragment_shader(
    VertexOut in [[stage_in]],
    constant ShadowData& shadowData [[buffer(23)]],
    constant PointShadowData& pointShadowData [[buffer(24)]],
    constant float& roughness [[buffer(25)]],
    constant float& metallic [[buffer(26)]],
    constant float4& cameraPosition [[buffer(27)]],
    constant Lights& lights [[buffer(28)]],
    constant CompactPlanetInfo& planetInfo [[buffer(13)]],
    constant BVHNode* data [[buffer(14)]],
    constant Triangle* primitives [[buffer(15)]],
    constant PotentialSamplingInfo& potentialSamplingInfo [[buffer(16)]],
    constant AtmosphereSettings& atmosphereSettings [[buffer(21)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    texture3d<float> densityTexture [[texture(4)]],
    texture3d<float> lightTransmittanceTexture [[texture(5)]],
    depth2d_array<float> shadowMaps [[texture(0)]],
    depthcube_array<float> cubeMaps [[texture(1)]],
    sampler planetSampler [[sampler(0)]],
    sampler densitySampler [[sampler(1)]]
) {
    auto geometricN = -normalize(cross(dfdx(in.worldPosition.xyz), dfdy(in.worldPosition.xyz)));
    auto eye = (in.worldPosition - cameraPosition).xyz;
    
    auto delta = max(length(dfdx(in.worldPosition)), length(dfdy(in.worldPosition))) / planetInfo.planetRadius / planetInfo.deltaMultiplier;
    delta = clamp(delta, planetInfo.minDelta, planetInfo.maxDelta);
    auto uv = fromPosToUV(normalize(in.icoPosition));
    
    auto baseNormal = normalTex.sample(planetSampler, uv).xyz;
    float3 N = baseNormal;
    
    float3 e1;
    float3 e2;
    int attempts = 1;
    float4 worldPos = float4(0.0, 0.0, 0.0, 1.0);
    
    while (attempts > 0) {
        attempts--;
        float2 uv_plus_u = float2(uv.x + delta, uv.y);
        float2 uv_plus_v = float2(uv.x, uv.y + delta);
        float2 uv_minus_u = float2(uv.x - delta, uv.y);
        float2 uv_minus_v = float2(uv.x, uv.y - delta);
            
        uint octaves = planetInfo.octaves;
        auto a = get_full_displaced_pos(fromUVToPos(uv_plus_u), planetInfo, planetTex, normalTex, planetSampler, octaves);
        auto b = get_full_displaced_pos(fromUVToPos(uv_minus_u), planetInfo, planetTex, normalTex, planetSampler, octaves);
        e1 = a - b;
        auto c = get_full_displaced_pos(fromUVToPos(uv_plus_v), planetInfo, planetTex, normalTex, planetSampler, octaves);
        auto d = get_full_displaced_pos(fromUVToPos(uv_minus_v), planetInfo, planetTex, normalTex, planetSampler, octaves);
        e2 = c - d;
        
        worldPos.xyz = (a + b + c + d) / 4.0;
            
        if (length(e1) < 1e-5) {
            delta *= 2;
            continue;
        }
        if (length(e2) < 1e-5) {
            delta *= 2;
            continue;
        }
        N = normalize(cross(e1, e2));
        if (dot(N, baseNormal) < 0) N = -N;
        break;
    }
    
    if (length(N) < 1e-9) {
        N = geometricN;
    }
    if (dot(N, eye) > 0 and dot(geometricN, eye) < 0) N = mix(N, geometricN, 0.5);
    
    float4 color = getRockyPlanetColor(worldPos.xyz, N);
    color = float4(0.5, 0.5, 0.5, 1.0);
    
    // ////////////////////////////////////////////////////////////////////////////////////
    // --- PRE-CALCOLO ATMOSFERA E TRASMITTANZA ---
    // ////////////////////////////////////////////////////////////////////////////////////
        
    float3 planetCenter = float3(0.0f, 0.0f, 0.0f);
    const float R_atmo = potentialSamplingInfo.nonZeroDensityRadius;
    float3 betaR = float3(0.148f, 0.344f, 0.844f);
    float3 I_sun = lights.directionalLights[0].color.xyz;

    float3 rayOrigin = cameraPosition.xyz;
    float3 rayDir = normalize(worldPos.xyz - cameraPosition.xyz);
    float3 L = normalize(-lights.directionalLights[0].direction);
    
    float t_atmo_entry = 0.0f;
    float t_atmo_exit = 0.0f;
    raySphereIntersect(rayOrigin, rayDir, planetCenter, R_atmo, t_atmo_entry, t_atmo_exit);

    float t_start = max(0.0f, t_atmo_entry);
    float t_end = distance(rayOrigin, worldPos.xyz);

    // Fissato a 0.5f come da tua richiesta per eliminare il rumore temporale a terra
    //auto jitter = interleavedGradientNoise(in.position.xy);
    auto jitter = smoothNoise(worldPos.xyz * 1000.0);
    if (not atmosphereSettings.jitter) jitter = 0.5f;

    float3 accumulatedScattering = float3(0.0f);
    float3 transmittanceCamera = float3(1.0f);
    float3 T_sun_to_surface = float3(1.0f);

    if (t_start < t_end) {
        const int SAMPLES = atmosphereSettings.SAMPLES;
        const int SUN_SAMPLES = atmosphereSettings.SUN_SAMPLES;
        float stepLength = (t_end - t_start) / float(SAMPLES);
            
        float mu = dot(rayDir, L);
        float phaseR = 0.75f * (1.0f + mu * mu) / (4.0f * 3.14159265f);

        for (int i = 0; i < SAMPLES; i++) {
            float t = t_start + stepLength * (float(i) + jitter);
            float3 P = rayOrigin + rayDir * t;
                    
            float3 uvw = (P - potentialSamplingInfo.min.xyz) / potentialSamplingInfo.edge;
                    
            float density = 0.0f;
            if (all(uvw >= 0.0f) && all(uvw <= 1.0f)) {
                density = sampleDensitySmooth3D(densityTexture, densitySampler, uvw);
            }

            float3 stepOpticalDepth = betaR * density * stepLength;
                    
            float3 T_sun;
            if (not atmosphereSettings.useBakedLightTransmittance) {
                T_sun = lightTransmittance(
                                                  P, L, potentialSamplingInfo, betaR, densityTexture, densitySampler, jitter, shadowMaps, shadowData, lights.numDirectionalLights, baseNormal, false, 1.0f, 0.01f, false, SUN_SAMPLES
                                                  );
            } else {
                T_sun = lightTransmittanceFromTexture(P, potentialSamplingInfo, lightTransmittanceTexture, densitySampler, betaR);
            }
                    
            accumulatedScattering += T_sun * transmittanceCamera * (betaR * density * stepLength) * phaseR * I_sun;
            transmittanceCamera *= exp(-stepOpticalDepth);
        }

        // Calcoliamo il filtraggio solare direttamente sulla superficie
        float3 T_sun_to_surface;
        if (not atmosphereSettings.useBakedLightTransmittance) {
            T_sun_to_surface = lightTransmittance(worldPos.xyz, L, potentialSamplingInfo, betaR, densityTexture, densitySampler, jitter, shadowMaps, shadowData, lights.numDirectionalLights, baseNormal, false, 1.0, 0.01f, false, SUN_SAMPLES
                                                  );
        } else {
            T_sun_to_surface = lightTransmittanceFromTexture(worldPos.xyz, potentialSamplingInfo, lightTransmittanceTexture, densitySampler, betaR);
        }
    }
    
    auto c = applyFULLSHADOWWARDLightsWithOrenNayar(
            {color, normalize(float4(N, 0.0)), worldPos},
            roughness,
            metallic,
            cameraPosition,
            lights,
            T_sun_to_surface,
            shadowData,
            pointShadowData,
            shadowMaps,
            cubeMaps,
            planetSampler,
            data,
            primitives,
            planetInfo.useRayTracingShadows,
            0.0f,
            0.01f, false
    );
        
    // --- BLENDING ATMOSFERICO FINALE ---
    if (t_start < t_end) {
            // Applichiamo l'estinzione della camera sulla luce riflessa dal terreno e sommiamo l'aria luminosa
        c.xyz = c.xyz * transmittanceCamera + accumulatedScattering;
    }
        
    // Dithering sicuro solo sui canali colore (evita di corrompere l'Alpha)
    c.xyz += (jitter - 0.5f) / 255.0f;
    
    auto tonemapped = ACESFilm(c.xyz);
    
    FragmentOut fOut;
    fOut.color = float4(tonemapped, 1.0);
    fOut.motionVector = motionVector(in.currentClipPosition, in.previousClipPosition);

    return fOut;
}
