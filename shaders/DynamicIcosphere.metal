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
    float3 normal;
    float4 color;
    float2 uv;
    float3 icoPosition;
};

struct PlanetInfo {
    int degreeU;
    int degreeV;
    int nCP_U;
    int nCP_V;
    float planetRadius;
    int usePositionTexture;
    int useNormalTexture;
    int constantLOD;
    int useConstantLOD;
    int showMesh;
    int isRocky;
    int fractalOctaves;
    float fractalIntensity;
    float fractalScale;
    int useRockyTexture;
    int quadrantX;
    int quadrantY;
    int rockyResolution;
};


float ridgedFBMTriplanar(float3 p, float3 normal, int octaves) {
    float3 blendWeights = abs(normal);
    blendWeights = blendWeights * blendWeights * blendWeights;
    blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);

    float noiseX = ridgedFBM(p.yzx, octaves);
    float noiseY = ridgedFBM(p.xzy, octaves);
    float noiseZ = ridgedFBM(p.xyz, octaves);

    return noiseX * blendWeights.x + noiseY * blendWeights.y + noiseZ * blendWeights.z;
}

// --- B-SPLINE LOGIC ---

float bspline_to_u(float t, constant int32_t* knots, int nKnots, int p) {
    return (1.0 - t) * (float)knots[p - 1] + t * (float)knots[nKnots - p];
}

int bspline_span(float t, constant int32_t* knots, int nKnots, int p) {
    float u = bspline_to_u(t, knots, nKnots, p);
    int span = p - 1;
    while (u >= knots[span + 1] && span < nKnots - p - 1) {
        ++span;
    }
    return span;
}

void bspline_basis(int span, float u, constant int32_t* knots, int p, thread float* basis) {
    float oldBasis[8];
    for (int j = 0; j <= p; j++) {
        basis[j] = 0.0;
        oldBasis[j] = 0.0;
    }
    basis[p] = 1.0;
    oldBasis[p] = 1.0;

    for (int i = 1; i <= p; i++) {
        for (int j = 0; j <= p; j++) {
            int b = j + span - p + 1;
            float w1 = 0.0;
            float w2 = 0.0;

            if (j > 0) {
                float d1 = (float)knots[b + i - 1] - (float)knots[b - 1];
                if (d1 != 0)
                    w1 = (u - (float)knots[b - 1]) / d1;
            }
            if (j < p) {
                float d2 = (float)knots[b + i] - (float)knots[b];
                if (d2 != 0)
                    w2 = ((float)knots[b + i] - u) / d2;
            }

            basis[j] = w1 * oldBasis[j];
            if (j < p)
                basis[j] += w2 * oldBasis[j + 1];
        }
        for (int j = 0; j <= p; j++) oldBasis[j] = basis[j];
    }
}

void bspline_d1basis(int span, float u, constant int32_t* knots, int p, thread float* basis) {
    float oldBasis[8];
    for (int j = 0; j <= p; j++) {
        basis[j] = 0.0;
        oldBasis[j] = 0.0;
    }
    basis[p] = 1.0;
    oldBasis[p] = 1.0;

    for (int i = 1; i < p; i++) {
        for (int j = 0; j <= p; j++) {
            int b = j + span - p + 1;
            float w1 = 0.0;
            float w2 = 0.0;
            if (j > 0) {
                float d1 = knots[b + i - 1] - knots[b - 1];
                if (d1 != 0) w1 = (u - knots[b - 1]) / d1;
            }
            if (j < p) {
                float d2 = knots[b + i] - knots[b];
                if (d2 != 0) w2 = (knots[b + i] - u) / d2;
            }
            basis[j] = w1 * oldBasis[j];
            if (j < p) basis[j] += w2 * oldBasis[j + 1];
        }
        for (int j = 0; j <= p; j++) oldBasis[j] = basis[j];
    }

    for (int j = 0; j <= p; j++) {
        int b = j + span - p + 1;
        float w1 = 0.0;
        float w2 = 0.0;
        if (j > 0) {
            float d1 = (float)knots[b + p - 1] - (float)knots[b - 1];
            if (d1 != 0) w1 = (float)p / d1;
        }
        if (j < p) {
            float d2 = (float)knots[b + p] - (float)knots[b];
            if (d2 != 0) w2 = (float)p / d2;
        }
        basis[j] = w1 * oldBasis[j];
        if (j < p) basis[j] -= w2 * oldBasis[j + 1];
    }
}

float3 bspline_u_derivative(float u_t, float v_t,
                             constant packed_float3* cp,
                             constant int32_t* knotsU,
                             constant int32_t* knotsV,
                             PlanetInfo info) {
    int nKnotsU = info.nCP_U + info.degreeU - 1;
    int nKnotsV = info.nCP_V + info.degreeV - 1;
    float u = bspline_to_u(u_t, knotsU, nKnotsU, info.degreeU);
    float v = bspline_to_u(v_t, knotsV, nKnotsV, info.degreeV);
    int uSpan = bspline_span(u_t, knotsU, nKnotsU, info.degreeU);
    int vSpan = bspline_span(v_t, knotsV, nKnotsV, info.degreeV);
    float uBasis[8], vBasis[8];
    bspline_d1basis(uSpan, u, knotsU, info.degreeU, uBasis);
    bspline_basis(vSpan, v, knotsV, info.degreeV, vBasis);
    float3 result = float3(0.0);
    for (int i = 0; i <= info.degreeV; i++) {
        for (int j = 0; j <= info.degreeU; j++) {
            int cpIdx = (vSpan - info.degreeV + 1 + i) * info.nCP_U + (uSpan - info.degreeU + 1 + j);
            result += uBasis[j] * vBasis[i] * float3(cp[cpIdx]);
        }
    }
    return result;
}

float3 bspline_v_derivative(float u_t, float v_t,
                             constant packed_float3* cp,
                             constant int32_t* knotsU,
                             constant int32_t* knotsV,
                             PlanetInfo info) {
    int nKnotsU = info.nCP_U + info.degreeU - 1;
    int nKnotsV = info.nCP_V + info.degreeV - 1;
    float u = bspline_to_u(u_t, knotsU, nKnotsU, info.degreeU);
    float v = bspline_to_u(v_t, knotsV, nKnotsV, info.degreeV);
    int uSpan = bspline_span(u_t, knotsU, nKnotsU, info.degreeU);
    int vSpan = bspline_span(v_t, knotsV, nKnotsV, info.degreeV);
    float uBasis[8], vBasis[8];
    bspline_basis(uSpan, u, knotsU, info.degreeU, uBasis);
    bspline_d1basis(vSpan, v, knotsV, info.degreeV, vBasis);
    float3 result = float3(0.0);
    for (int i = 0; i <= info.degreeV; i++) {
        for (int j = 0; j <= info.degreeU; j++) {
            int cpIdx = (vSpan - info.degreeV + 1 + i) * info.nCP_U + (uSpan - info.degreeU + 1 + j);
            result += uBasis[j] * vBasis[i] * float3(cp[cpIdx]);
        }
    }
    return result;
}

float3 bspline_evaluate(float u_t, float v_t, 
                         constant packed_float3* cp,
                         constant int32_t* knotsU,
                         constant int32_t* knotsV,
                         PlanetInfo info) {
    int nKnotsU = info.nCP_U + info.degreeU - 1;
    int nKnotsV = info.nCP_V + info.degreeV - 1;

    float u = bspline_to_u(u_t, knotsU, nKnotsU, info.degreeU);
    float v = bspline_to_u(v_t, knotsV, nKnotsV, info.degreeV);

    int uSpan = bspline_span(u_t, knotsU, nKnotsU, info.degreeU);
    int vSpan = bspline_span(v_t, knotsV, nKnotsV, info.degreeV);

    float uBasis[8];
    float vBasis[8];
    bspline_basis(uSpan, u, knotsU, info.degreeU, uBasis);
    bspline_basis(vSpan, v, knotsV, info.degreeV, vBasis);

    float3 result = float3(0.0);
    for (int i = 0; i <= info.degreeV; i++) {
        for (int j = 0; j <= info.degreeU; j++) {
            int cpIdx = (vSpan - info.degreeV + 1 + i) * info.nCP_U + (uSpan - info.degreeU + 1 + j);
            result += uBasis[j] * vBasis[i] * float3(cp[cpIdx]);
        }
    }
    return result;
}

float3 calculate_normal(float u_t, float v_t,
                        constant packed_float3* cp,
                        constant int32_t* knotsU,
                        constant int32_t* knotsV,
                        PlanetInfo info) {
    float eps = 1e-5;
    if (v_t < eps) {
        float3 res = float3(0.0);
        float closeV = eps;
        for (int i = 1; i <= 8; i++) {
            float ut = 1.0 / (float)i;
            res += cross(bspline_u_derivative(ut, closeV, cp, knotsU, knotsV, info),
                         bspline_v_derivative(ut, closeV, cp, knotsU, knotsV, info));
        }
        float3 n = res;
        float3 p = bspline_evaluate(u_t, closeV, cp, knotsU, knotsV, info);
        if (dot(n, p) < 0.0) n = -n;
        return normalize(n);
    }
    if (v_t > 1.0 - eps) {
        float3 res = float3(0.0);
        float closeV = 1.0 - eps;
        for (int i = 1; i <= 8; i++) {
            float ut = 1.0 / (float)i;
            res += cross(bspline_u_derivative(ut, closeV, cp, knotsU, knotsV, info),
                         bspline_v_derivative(ut, closeV, cp, knotsU, knotsV, info));
        }
        float3 n = res;
        float3 p = bspline_evaluate(u_t, closeV, cp, knotsU, knotsV, info);
        if (dot(n, p) < 0.0) n = -n;
        return normalize(n);
    }

    float3 du = bspline_u_derivative(u_t, v_t, cp, knotsU, knotsV, info);
    float3 dv = bspline_v_derivative(u_t, v_t, cp, knotsU, knotsV, info);
    float3 n = cross(du, dv);
    
    // Ensure the normal points outwards from the planet center
    float3 p = bspline_evaluate(u_t, v_t, cp, knotsU, knotsV, info);
    if (dot(n, p) < 0.0) {
        n = -n;
    }
    
    // Safety check for invalid normals
    if (length_squared(n) < 1e-6) {
        return normalize(p);
    }
    
    return normalize(n);
}


float3 get_planet_pos(float3 p_ico, constant packed_float3* cp, constant int32_t* knotsU, constant int32_t* knotsV, PlanetInfo info, texture2d<float> planetTex, texture2d<float> normalTex, sampler s) {
    float phi = atan2(-p_ico.x, p_ico.z);
    float theta = acos(clamp(p_ico.y, -1.0f, 1.0f));
    float u_planet = (phi + M_PI_F) / (2.0 * M_PI_F);
    float v_planet = clamp(theta / M_PI_F, 0.0, 1.0);

    float3 p;
    if (info.usePositionTexture) {
        p = planetTex.sample(s, float2(u_planet, v_planet)).xyz;
    } else if (info.nCP_U > 0 && info.nCP_V > 0) {
        p = bspline_evaluate(u_planet, v_planet, cp, knotsU, knotsV, info);
    } else {
        p = p_ico;
    }
    return p;
}

float sampleRocky(float2 uv, texture2d<float> rTL, texture2d<float> rTR, texture2d<float> rBL, texture2d<float> rBR, sampler s) {
    //uv = float2(clamp(uv.x, 0.0, 1.0), clamp(uv.y, 0.0, 1.0));
    if (uv.x < 0.5) {
        if (uv.y < 0.5) {
            return rTL.sample(s, uv * 2.0).r;
        } else {
            return rBL.sample(s, (uv - float2(0.0, 0.5)) * 2.0).r;
        }
    } else {
        if (uv.y < 0.5) {
            return rTR.sample(s, (uv - float2(0.5, 0.0)) * 2.0).r;
        } else {
            return rBR.sample(s, (uv - float2(0.5, 0.5)) * 2.0).r;
        }
    }
}

float3 get_full_displaced_pos(float3 p_ico, constant packed_float3* cp, constant int32_t* knotsU, constant int32_t* knotsV, PlanetInfo info, texture2d<float> planetTex, texture2d<float> normalTex, texture2d<float> rTL, texture2d<float> rTR, texture2d<float> rBL, texture2d<float> rBR, sampler planetSampler, sampler rockySampler, uint octaves, bool useRockyTexture = true) {
    float phi = atan2(-p_ico.x, p_ico.z);
    float theta = acos(clamp(p_ico.y, -1.0f, 1.0f));
    float u_planet = (phi + M_PI_F) / (2.0 * M_PI_F);
    float v_planet = clamp(theta / M_PI_F, 0.0, 1.0);

    float3 p;
    float3 n_base;
    if (info.usePositionTexture) {
        p = planetTex.sample(planetSampler, float2(u_planet, v_planet)).xyz;
    } else if (info.nCP_U > 0 && info.nCP_V > 0) {
        p = bspline_evaluate(u_planet, v_planet, cp, knotsU, knotsV, info);
    } else {
        p = p_ico;
    }

    if (info.useNormalTexture) {
        n_base = normalTex.sample(planetSampler, float2(u_planet, v_planet)).xyz;
    } else if (info.nCP_U > 0 && info.nCP_V > 0) {
        n_base = normalize(p); // approximation
    } else {
        n_base = p_ico;
    }

    if (info.isRocky) {
        float3 n_eff = normalize(n_base);
        float displacement;
        if (info.useRockyTexture and useRockyTexture) {
             displacement = sampleRocky(float2(u_planet, v_planet), rTL, rTR, rBL, rBR, rockySampler);
        } else {
            displacement = ridgedFBM(p * info.fractalScale, octaves);
        }
        p += n_eff * displacement * info.fractalIntensity;
    }

    return p;
}


uint calculateLOD(float3 v0, float3 v1, float3 v2, float4x4 modelMatrix, float3 cameraPos,
                  float4x4 projectionMatrix,
                  constant packed_float3* cp,
                  constant int32_t* knotsU,
                  constant int32_t* knotsV,
                  PlanetInfo info,
                  texture2d<float> planetTex,
                  texture2d<float> normalTex,
                  sampler planetSampler,
                  sampler rockySampler,
                  texture2d<float> TL,
                  texture2d<float> TR,
                  texture2d<float> BL,
                  texture2d<float> BR) {

    if (info.useConstantLOD) {
        return uint(clamp(float(info.constantLOD), 0.0, 9.0));
    }

    // 1. Usa il raggio nominale del pianeta per la coerenza spaziale.
    // Non usare get_planet_pos qui, altrimenti il displacement della B-Spline
    // cambierà il LOD in modo imprevedibile.
    float radius = info.planetRadius > 0.0 ? info.planetRadius : 1.0;

    v0 = normalize(v0);
    v1 = normalize(v1);
    v2 = normalize(v2);
    auto vCenter = normalize((v0 + v1 + v2) / 3.0);

    auto wCenter = get_full_displaced_pos(vCenter, cp, knotsU, knotsV, info, planetTex, normalTex, TL, TR, BL, BR, planetSampler, rockySampler, 9);
    auto w0 = get_full_displaced_pos(v0, cp, knotsU, knotsV, info, planetTex, normalTex, TL, TR, BL, BR, planetSampler, rockySampler, 9);
    auto w1 = get_full_displaced_pos(v1, cp, knotsU, knotsV, info, planetTex, normalTex, TL, TR, BL, BR, planetSampler, rockySampler, 9);
    auto w2 = get_full_displaced_pos(v2, cp, knotsU, knotsV, info, planetTex, normalTex, TL, TR, BL, BR, planetSampler, rockySampler, 9);


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
    float projectedSize = (radius / 10.0 * screenRef) / dist;

    // 7. Mappatura con Curvatura
    // Portiamo il valore in un range normalizzato 0.0 - 1.0 basato sul range desiderato (0-9)
    float baseLod = log2(projectedSize * 32.0);
    float normalizedLod = clamp(baseLod / 9.0, 0.0, 1.0);

    // Applichiamo una curva di potenza:
    // 1.0 = lineare (come ora)
    // 2.0 = parabola (molto lento all'inizio, esplode alla fine)
    // 1.5 = una via di mezzo corretta
    float curvedLod = pow(normalizedLod, 1.5);

    // Riportiamo nel range 0-9
    float lodWeight = curvedLod * 9.0;
    
    return uint(clamp(lodWeight, 2.0, 9.0));
}
 

/*
// Funzione helper per calcolare il LOD di un segmento specifico
uint getEdgeLOD(float3 vA, float3 vB, float4x4 modelMatrix, float3 cameraPos, float4x4 projectionMatrix,
                constant packed_float3* cp, constant int32_t* knotsU, constant int32_t* knotsV, PlanetInfo info,
                texture2d<float> planetTex, sampler s) {
    float3 midPoint = normalize((vA + vB) * 0.5f);
    return calculateLOD(midPoint, midPoint, midPoint, modelMatrix, cameraPos, projectionMatrix, cp, knotsU, knotsV, info, planetTex, s);
}
 */


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
void dynamic_icosphere_object_shader(
    uint gid [[threadgroup_position_in_grid]],
    uint tid [[thread_index_in_threadgroup]],
    object_data Payload& payload [[payload]],
    mesh_grid_properties grid,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewMatrix [[buffer(26)]],
    constant float4x4& projectionMatrix [[buffer(27)]],
    constant float4& cameraPosition [[buffer(25)]],
    constant packed_float3* planetCP [[buffer(10)]],
    constant int32_t* planetKnotsU [[buffer(11)]],
    constant int32_t* planetKnotsV [[buffer(12)]],
    constant PlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    texture2d<float> rTL [[texture(5)]],
    texture2d<float> rTR [[texture(6)]],
    texture2d<float> rBL [[texture(7)]],
    texture2d<float> rBR [[texture(8)]],
    sampler planetSampler [[sampler(0)]],
    sampler rockySampler [[sampler(1)]]
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
        p[i] = get_planet_pos(sv[i], planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, planetSampler);
    }
    float3 vCenter = normalize(sv[0] + sv[1] + sv[2]);
    p[3] = get_planet_pos(vCenter, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, planetSampler);

    float3 w[4];
    for (int i = 0; i < 4; ++i) {
        w[i] = (modelMatrix * float4(p[i], 1.0)).xyz;
    }
    
    float3 faceNormal = normalize(cross(w[1] - w[0], w[2] - w[0]));
    float3 viewDir = normalize(w[3] - cameraPosition.xyz);
    
    // Horizon Culling: se il dot tra la normale del centro (mondo) e la direzione verso la camera è > 0, 
    // il meshlet è dietro l'orizzonte.
    // Usiamo il worldCenter normalizzato come approssimazione della normale alla superficie del pianeta.
    //if (dot(normalize(w[3]), normalize(w[3] - cameraPosition.xyz)) > 0.0) return;

    if (dot(faceNormal, viewDir) > 0.2) return;

    float4 clipPos[4];
    float4x4 vp = projectionMatrix * viewMatrix;
    bool allOutside[6] = {true, true, true, true, true, true};
    float margin = 0.0;

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

    uint subd = calculateLOD(sv[0], sv[1], sv[2], modelMatrix, cameraPosition.xyz, projectionMatrix, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, planetSampler, rockySampler, rTL, rTR, rBL, rBR);
    
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
void dynamic_icosphere_mesh_shader(
    uint tid [[thread_index_in_threadgroup]],
    uint mid [[threadgroup_position_in_grid]],
    const object_data Payload& payload [[payload]],
    mesh<VertexOut, void, 256, 512, topology::triangle> output,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewMatrix [[buffer(26)]],
    constant float4x4& projectionMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant packed_float3* planetCP [[buffer(10)]],
    constant int32_t* planetKnotsU [[buffer(11)]],
    constant int32_t* planetKnotsV [[buffer(12)]],
    constant PlanetInfo& planetInfo [[buffer(13)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    texture2d<float> rTL [[texture(5)]],
    texture2d<float> rTR [[texture(6)]],
    texture2d<float> rBL [[texture(7)]],
    texture2d<float> rBR [[texture(8)]],
    sampler planetSampler [[sampler(0)]],
    sampler rockySampler [[sampler(1)]]
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

    bool isActuallyVisible = (planetInfo.showMesh != 0);
    
    // make frustum culling
    if (tid == 0 && isActuallyVisible) {
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
        float margin = 0.1;

        for (int i = 0; i < 3; ++i) {
            float3 p_ico = normalize(faceV0 * corners_b[i].x + faceV1 * corners_b[i].y + faceV2 * corners_b[i].z);
            float3 p_planet;
            if (!planetInfo.isRocky) {
                p_planet = get_planet_pos(p_ico, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, planetSampler);
            } else {
                p_planet = get_full_displaced_pos(p_ico, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, payload.subdivisionLevel);
            }
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

        /*
        // 1. Definisci una piccola tolleranza per identificare i bordi
        float eps = 1e-3;

        // 2. Calcola il LOD effettivo dei bordi basandoti sui dati passati dal Payload
        // Il segreto è che sf.edgeLOD[i] deve essere il MINIMO tra il LOD di questa subface
        // e quello della subface adiacente.
        uint targetLOD0 = min(sf.subdivisionLevel, sf.edgeLOD[0]);
        uint targetLOD1 = min(sf.subdivisionLevel, sf.edgeLOD[1]);
        uint targetLOD2 = min(sf.subdivisionLevel, sf.edgeLOD[2]);

        if (w < eps && targetLOD0 < sf.subdivisionLevel) {
            // Bordo v0-v1: Forziamo il vertice sulla griglia del LOD più basso
            float segments_low = float(1 << targetLOD0);
            float t = round(u_b * segments_low) / segments_low;
            p_ico = normalize(mix(v0, v1, t));
        }
        else if (v_b < eps && targetLOD1 < sf.subdivisionLevel) {
            // Bordo v1-v2
            float segments_low = float(1 << targetLOD1);
            float t = round(w * segments_low) / segments_low;
            p_ico = normalize(mix(v1, v2, t));
        }
        else if (u_b < eps && targetLOD2 < sf.subdivisionLevel) {
            // Bordo v2-v0
            float segments_low = float(1 << targetLOD2);
            float t = round(v_b * segments_low) / segments_low;
            p_ico = normalize(mix(v2, v0, t));
        }
         */

        
        
        VertexOut vout;
        
        auto uv = fromPosToUV(p_ico);
        vout.icoPosition = p_ico;
        
        if (planetInfo.useNormalTexture) {
            vout.normal = normalTex.sample(planetSampler, uv).xyz;
        } else if (planetInfo.nCP_U > 0 && planetInfo.nCP_V > 0) {
            vout.normal = calculate_normal(uv.x, uv.y, planetCP, planetKnotsU, planetKnotsV, planetInfo); // approximation, but better than p_ico
        } else {
            vout.normal = p_ico;
        }
        
        float3 p;
        //if (planetInfo.isRocky) {
            p = get_full_displaced_pos(p_ico, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, payload.subdivisionLevel);
            /*
            auto uv = fromPosToUV(p_ico);
            auto uv_plus_u = float2(uv.x + planetInfo.normalDelta, uv.y);
            auto uv_plus_v = float2(uv.x, uv.y + planetInfo.normalDelta);
            auto uv_minus_u = float2(uv.x - planetInfo.normalDelta, uv.y);
            auto uv_minus_v = float2(uv.x, uv.y - planetInfo.normalDelta);
            auto e1 = get_full_displaced_pos(fromUVToPos(uv_plus_u), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, 9) - get_full_displaced_pos(fromUVToPos(uv_minus_u), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, 9);
            auto e2 = get_full_displaced_pos(fromUVToPos(uv_plus_v), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, 9) - get_full_displaced_pos(fromUVToPos(uv_minus_v), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, 9);
            vout.normal = normalize(cross(e1, e2));
             */
            /*
        } else {
            p = get_planet_pos(p_ico, planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, planetSampler);
        }*/
        
        float4 worldPos = modelMatrix * float4(p, 1.0);
        vout.position = viewProjectionMatrix * worldPos;
        vout.worldPosition = worldPos;
        vout.uv = fromPosToUV(p_ico);

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

// --- FRAGMENT SHADER ---
[[fragment]]
float4 dynamic_icosphere_fragment_shader(
    VertexOut in [[stage_in]],
    constant ShadowData& shadowData [[buffer(23)]],
    constant PointShadowData& pointShadowData [[buffer(24)]],
    constant float& roughness [[buffer(25)]],
    constant float& metallic [[buffer(26)]],
    constant float4& cameraPosition [[buffer(27)]],
    constant Lights& lights [[buffer(28)]],
    constant PlanetInfo& planetInfo [[buffer(13)]],
    constant packed_float3* planetCP [[buffer(10)]],
    constant int32_t* planetKnotsU [[buffer(11)]],
    constant int32_t* planetKnotsV [[buffer(12)]],
    texture2d<float> planetTex [[texture(2)]],
    texture2d<float> normalTex [[texture(3)]],
    texture2d<float> rTL [[texture(5)]],
    texture2d<float> rTR [[texture(6)]],
    texture2d<float> rBL [[texture(7)]],
    texture2d<float> rBR [[texture(8)]],
    depth2d_array<float> shadowMaps [[texture(0)]],
    depthcube_array<float> cubeMaps [[texture(1)]],
    sampler planetSampler [[sampler(0)]],
    sampler rockySampler [[sampler(1)]]
) {
    
    auto delta = max(length(dfdx(in.uv)), length(dfdy(in.uv)));
    delta = clamp(delta, 0.0000001, 0.001);
    auto uv = fromPosToUV(normalize(in.icoPosition));
    
    auto baseNormal = normalTex.sample(planetSampler, uv).xyz;
    float3 N = baseNormal;
    
    float3 e1;
    float3 e2;
    int attempts = 10;
    if (planetInfo.isRocky){//} && planetInfo.useRockyTexture) {
        while (attempts > 0) {
            attempts--;
            float2 uv_plus_u = float2(uv.x + delta, uv.y);
            float2 uv_plus_v = float2(uv.x, uv.y + delta);
            float2 uv_minus_u = float2(uv.x - delta, uv.y);
            float2 uv_minus_v = float2(uv.x, uv.y - delta);
            
            e1 = get_full_displaced_pos(fromUVToPos(uv_plus_u), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, planetInfo.fractalOctaves, false) -
            get_full_displaced_pos(fromUVToPos(uv_minus_u), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, planetInfo.fractalOctaves, false);
            e2 = get_full_displaced_pos(fromUVToPos(uv_plus_v), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, planetInfo.fractalOctaves, false) -
            get_full_displaced_pos(fromUVToPos(uv_minus_v), planetCP, planetKnotsU, planetKnotsV, planetInfo, planetTex, normalTex, rTL, rTR, rBL, rBR, planetSampler, rockySampler, planetInfo.fractalOctaves, false);
            
            if (length(e1) < 1e-9) {
                delta *= 2;
                continue;
            }
            if (length(e2) < 1e-9) {
                delta *= 2;
                continue;
            }
            N = normalize(cross(e1, e2));
            break;
        }
    }
    
    float4 color = getRockyPlanetColor(baseNormal, N);
    
    return applyFULLSHADOWWARDLights(
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
}




[[mesh]]
void planet_debug_mesh_shader(
    uint tid [[thread_index_in_threadgroup]],
    uint mid [[threadgroup_position_in_grid]],
    mesh<VertexOut, void, 256, 512, topology::triangle> output,
    constant float4x4& modelMatrix [[buffer(22)]],
    constant float4x4& viewMatrix [[buffer(26)]],
    constant float4x4& projectionMatrix [[buffer(27)]],
    constant packed_float3* planetCP [[buffer(10)]],
    constant int32_t* planetKnotsU [[buffer(11)]],
    constant int32_t* planetKnotsV [[buffer(12)]],
    constant PlanetInfo& planetInfo [[buffer(13)]]
) {
    
    if (planetInfo.nCP_U <= 0 || planetInfo.nCP_V <= 0) return;

    // Disegniamo una striscia della superficie B-spline per meshlet (lungo U, per un range V fisso)
    // mid rappresenta l'indice della striscia V
    uint nU = 32; // Numero di suddivisioni in U
    
    uint numTriangles = nU * 2;

    if (tid == 0) {
        output.set_primitive_count(numTriangles);
    }

    float v_step = 1.0 / 31.0;
    float v0_t = (float)mid * v_step;
    float v1_t = (float)(mid + 1) * v_step;

    for (uint i = tid; i <= nU; i += 64) {
        float u_t = (float)i / (float)nU;
        u_t = clamp(u_t, 0.0, 1.0);
        
        // Vertice per v0
        float3 p0 = bspline_evaluate(u_t, v0_t, planetCP, planetKnotsU, planetKnotsV, planetInfo);
        float3 n0 = calculate_normal(u_t, v0_t, planetCP, planetKnotsU, planetKnotsV, planetInfo);
        
        VertexOut vout0;
        float4 worldPos0 = modelMatrix * float4(p0, 1.0);
        vout0.position = projectionMatrix * viewMatrix * worldPos0;
        vout0.worldPosition = worldPos0;
        vout0.normal = n0;
        vout0.color = float4(0.0, 1.0, 0.0, 1.0); // Verde per il debug
        vout0.uv = float2(u_t, v0_t);
        output.set_vertex(i * 2, vout0);

        // Vertice per v1
        float3 p1 = bspline_evaluate(u_t, v1_t, planetCP, planetKnotsU, planetKnotsV, planetInfo);
        float3 n1 = calculate_normal(u_t, v1_t, planetCP, planetKnotsU, planetKnotsV, planetInfo);
        
        VertexOut vout1;
        float4 worldPos1 = modelMatrix * float4(p1, 1.0);
        vout1.position = projectionMatrix * viewMatrix * worldPos1;
        vout1.worldPosition = worldPos1;
        vout1.normal = n1;
        vout1.color = float4(0.0, 0.8, 0.0, 1.0);
        vout1.uv = float2(u_t, v1_t);
        output.set_vertex(i * 2 + 1, vout1);
    }

    threadgroup_barrier(mem_flags::mem_none);
    
    for (uint i = tid; i < nU; i += 64) {
        // Triangolo 1
        output.set_index(i * 6 + 0, i * 2);
        output.set_index(i * 6 + 1, (i + 1) * 2);
        output.set_index(i * 6 + 2, i * 2 + 1);
        
        // Triangolo 2
        output.set_index(i * 6 + 3, (i + 1) * 2);
        output.set_index(i * 6 + 4, (i + 1) * 2 + 1);
        output.set_index(i * 6 + 5, i * 2 + 1);
    }
}
