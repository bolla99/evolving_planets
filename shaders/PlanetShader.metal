#include <metal_stdlib>
#include "Lighting.hpp"
#include "util.hpp"
using namespace metal;

struct Payload {
    uint subdivisionLevel;
    float3 v0, v1, v2;
    float4 color;
};

struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float2 uv;
    float3 icoPosition;
    uint subd [[flat]];
};

struct CompactPlanetInfo {
    float planetRadius;
    float fractalIntensity;
    float fractalScale;
    int useConstantLOD;
    int constantLOD;
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

    // 7. Mappatura con Curvatura
    // Portiamo il valore in un range normalizzato 0.0 - 1.0 basato sul range desiderato (0-9)
    float baseLod = log2(projectedSize * 64.0);
    float normalizedLod = clamp(baseLod / 9.0, 0.0, 1.0);

    // Applichiamo una curva di potenza:
    // 1.0 = lineare (come ora)
    // 2.0 = parabola (molto lento all'inizio, esplode alla fine)
    // 1.5 = una via di mezzo corretta
    float curvedLod = pow(normalizedLod, 1.5);

    // Riportiamo nel range 0-9
    float lodWeight = curvedLod * 9.0;
    
    return uint(clamp(lodWeight, 1.0, 9.0));
}

constant float occlusionMargin = 0.1f;

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
        subd = calculateLOD(sv[0], sv[1], sv[2], modelMatrix, cameraPosition.xyz, projectionMatrix, planetInfo, planetTex, normalTex, planetSampler, 0);
    }
    
    uint numMeshlets = 1;
    if (subd > 4) {
        numMeshlets = 1 << (subd - 4);
        numMeshlets *= numMeshlets;
    }

    payload.subdivisionLevel = subd;
    payload.v0 = sv[0];
    payload.v1 = sv[1];
    payload.v2 = sv[2];
    
    grid.set_threadgroups_per_grid(uint3(numMeshlets, 1, 1));
}

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
    uint subd = payload.subdivisionLevel;
    uint n = 1 << subd;
    
    uint m = 1;
    if (subd > 4) m = 1 << (subd - 4);
    
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

    uint numTriangles = n_tile * n_tile;
    float3 faceV0 = payload.v0;
    float3 faceV1 = payload.v1;
    float3 faceV2 = payload.v2;

    bool isActuallyVisible = true;
    
    // make frustum culling
    if (tid == 0) {
        // Angoli del meshlet in coordinate baricentriche della subface
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
            float3 p_ico = normalize(faceV0 * corners_b[i].x + faceV1 * corners_b[i].y + faceV2 * corners_b[i].z);
            auto p_planet = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 2);
            float4 clipPos = vp * modelMatrix * float4(p_planet, 1.0);
            
            float w_margin = clipPos.w * (1.0 + margin);
            if (clipPos.x >= -w_margin) allOutside[0] = false;
            if (clipPos.x <=  w_margin) allOutside[1] = false;
            if (clipPos.y >= -w_margin) allOutside[2] = false;
            if (clipPos.y <=  w_margin) allOutside[3] = false;
            if (clipPos.z >= 0) allOutside[4] = false;
            if (clipPos.z <=  w_margin) allOutside[5] = false;
        }

        for (int j = 0; j < 6; ++j) {
            if (allOutside[j]) { isActuallyVisible = false; break; }
        }

        if (!isActuallyVisible) {
            numTriangles = 0;
            output.set_primitive_count(0);
        } else {
            output.set_primitive_count(numTriangles);
        }
    }
    
    // wait for all threads (actually only thread 0 performed the frustum culling
    threadgroup_barrier(mem_flags::mem_none);
    // Nota: in Metal mesh shader non esiste un modo diretto per leggere primitive_count
    // dagli altri thread in modo portabile senza payload o variabili threadgroup.
    // Ma in questo caso, se il meshlet è scartato via set_primitive_count(0),
    // possiamo semplicemente lasciare che i thread facciano il lavoro se non abbiamo
    // un modo pulito per uscire, oppure usare una variabile threadgroup.
    
    threadgroup bool isCulled = false;
    if (tid == 0) {
        isCulled = !isActuallyVisible;
    }
    // every thread wait for the thread 0 to set culled flag to the right value
    threadgroup_barrier(mem_flags::mem_threadgroup);
    // every thread can return
    if (isCulled) return;
    
    // these values are constant: each meshlet is always a triangle subdivided 4 times
    // numVertices per meshlet
    uint numVertices = (n_tile + 1) * (n_tile + 2) / 2;

    uint numThreads = 64;
    // inside each meshlet / threadgroup, each thread handles a single vertex
    // Vertex generation for the tile
    for (uint vIdx = tid; vIdx < numVertices; vIdx += numThreads) {
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
        
        float w, u_b;
        if (!downward) {
            w = (float)m_row / m + (float)i_tile / n;
            u_b = (float)(m_col / 2) / m + (float)j_tile / n;
        } else {
            w = (float)(m_row + 1) / m - (float)j_tile / n;
            u_b = (float)(m_col / 2 + 1) / m - (float)i_tile / n;
        }
        float v_b = 1.0 - w - u_b;

        float3 p_ico;
        p_ico = normalize(faceV0 * v_b + faceV1 * u_b + faceV2 * w);

        VertexOut vout;
        vout.uv = fromPosToUV(p_ico);
                
        auto p = get_full_displaced_pos(p_ico, planetInfo, planetTex, normalTex, planetSampler, 9);
        
        //auto n = normalTex.sample(planetSampler, vout.uv).xyz;
        
        /*
        if (planetInfo.useSkirts != 0) {
            bool isEdge = (i_tile == 0) || (j_tile == 0) || (i_tile + j_tile == n_tile);
            if (isEdge) {
                float skirtDepth = planetInfo.planetRadius * 0.001; // 5% del raggio
                p -= n * skirtDepth;
            }
        }*/

        float4 worldPos = modelMatrix * float4(p, 1.0);
        vout.position = viewProjectionMatrix * worldPos;
        vout.worldPosition = worldPos;
        vout.icoPosition = p_ico;
        vout.subd = payload.subdivisionLevel;

        output.set_vertex(vIdx, vout);
    }

    // Index generation for the tile
    for (uint tIdx = tid; tIdx < numTriangles; tIdx += numThreads) {
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


float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return saturate((x * (a * x + b)) / (x * (c * x + d) + e));
}


// --- FRAGMENT SHADER ---
[[fragment]]
float4 planet_fragment_shader(
    VertexOut in [[stage_in]],
    constant ShadowData& shadowData [[buffer(23)]],
    constant PointShadowData& pointShadowData [[buffer(24)]],
    constant float& roughness [[buffer(25)]],
    constant float& metallic [[buffer(26)]],
    constant float4& cameraPosition [[buffer(27)]],
    constant Lights& lights [[buffer(28)]],
    constant CompactPlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    depth2d_array<float> shadowMaps [[texture(0)]],
    depthcube_array<float> cubeMaps [[texture(1)]],
    sampler planetSampler [[sampler(0)]]
) {
    auto geometricN = -normalize(cross(dfdx(in.worldPosition.xyz), dfdy(in.worldPosition.xyz)));
    auto eye = (in.worldPosition - cameraPosition).xyz;
    
    auto delta = max(length(dfdx(in.uv)), length(dfdy(in.uv)));
    delta = clamp(delta, 0.0000001, 0.001);
    auto uv = fromPosToUV(normalize(in.icoPosition));
    
    auto baseNormal = normalTex.sample(planetSampler, uv).xyz;
    float3 N = baseNormal;
    
    float3 e1;
    float3 e2;
    int attempts = 5;
    while (attempts > 0) {
        attempts--;
        float2 uv_plus_u = float2(uv.x + delta, uv.y);
        float2 uv_plus_v = float2(uv.x, uv.y + delta);
        float2 uv_minus_u = float2(uv.x - delta, uv.y);
        float2 uv_minus_v = float2(uv.x, uv.y - delta);
            
        uint octaves = 14;
        e1 = get_full_displaced_pos(fromUVToPos(uv_plus_u), planetInfo, planetTex, normalTex, planetSampler, octaves) - get_full_displaced_pos(fromUVToPos(uv_minus_u), planetInfo, planetTex, normalTex, planetSampler, octaves);
        e2 = get_full_displaced_pos(fromUVToPos(uv_plus_v), planetInfo, planetTex, normalTex, planetSampler, octaves) - get_full_displaced_pos(fromUVToPos(uv_minus_v), planetInfo, planetTex, normalTex, planetSampler, octaves);
            
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
    
    float4 color = getRockyPlanetColor(in.worldPosition.xyz, N);
    
    auto c = applyFULLSHADOWWARDLightsWithOrenNayar(
        in.worldPosition,
        color,
        normalize(float4(N, 0.0)),
        roughness,
        metallic,
        cameraPosition,
        lights,
        shadowData,
        pointShadowData,
        shadowMaps,
        cubeMaps,
        planetSampler
    );
    
    auto tonemapped = ACESFilm(c.xyz);
    
    return float4(tonemapped, 1.0);
}
