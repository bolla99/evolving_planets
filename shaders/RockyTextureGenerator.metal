#include <metal_stdlib>
#include "util.hpp"
using namespace metal;

struct RockyConfig {
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
    float normalDelta;
    int useRockyTexture;
    int useSimplexNoise;
    int quadrantX;
    int quadrantY;
    int rockyResolution;
    int useTriplanar;
};

// --- B-SPLINE LOGIC ---

float bspline_to_u_local(float t, constant int32_t* knots, int nKnots, int p) {
    return (1.0 - t) * (float)knots[p - 1] + t * (float)knots[nKnots - p];
}

int bspline_span_local(float t, constant int32_t* knots, int nKnots, int p) {
    float u = bspline_to_u_local(t, knots, nKnots, p);
    int span = p - 1;
    while (u >= knots[span + 1] && span < nKnots - p - 1) {
        ++span;
    }
    return span;
}

void bspline_basis_local(int span, float u, constant int32_t* knots, int p, thread float* basis) {
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

float3 bspline_evaluate_local(float u_t, float v_t, 
                         constant packed_float3* cp,
                         constant int32_t* knotsU,
                         constant int32_t* knotsV,
                         RockyConfig info) {
    int nKnotsU = info.nCP_U + info.degreeU - 1;
    int nKnotsV = info.nCP_V + info.degreeV - 1;

    float u = bspline_to_u_local(u_t, knotsU, nKnotsU, info.degreeU);
    float v = bspline_to_u_local(v_t, knotsV, nKnotsV, info.degreeV);

    int uSpan = bspline_span_local(u_t, knotsU, nKnotsU, info.degreeU);
    int vSpan = bspline_span_local(v_t, knotsV, nKnotsV, info.degreeV);

    float uBasis[8];
    float vBasis[8];
    bspline_basis_local(uSpan, u, knotsU, info.degreeU, uBasis);
    bspline_basis_local(vSpan, v, knotsV, info.degreeV, vBasis);

    float3 result = float3(0.0);
    for (int i = 0; i <= info.degreeV; i++) {
        for (int j = 0; j <= info.degreeU; j++) {
            int cpIdx = (vSpan - info.degreeV + 1 + i) * info.nCP_U + (uSpan - info.degreeU + 1 + j);
            result += uBasis[j] * vBasis[i] * float3(cp[cpIdx]);
        }
    }
    return result;
}

// --- ROCKY LOGIC ---

float ridgedFBM_local(float3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 3.0f;
    float weight = 1.0f;

    for (int i = 0; i < octaves; i++) {
        auto n = smoothNoise(p * frequency);
        n = 1.0f - abs(n);
        n *= n;

        value += n * amplitude * weight;
        weight = n;

        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

float ridgedFBMTriplanar_local(float3 p, float3 normal, int octaves, int useSimplex) {
    float3 blendWeights = abs(normal);
    blendWeights = blendWeights * blendWeights * blendWeights;
    blendWeights /= (blendWeights.x + blendWeights.y + blendWeights.z);

    float noiseX = ridgedFBM_local(p.yzx, octaves);
    float noiseY = ridgedFBM_local(p.xzy, octaves);
    float noiseZ = ridgedFBM_local(p.xyz, octaves);

    return noiseX * blendWeights.x + noiseY * blendWeights.y + noiseZ * blendWeights.z;
}

kernel void generateRockyTexture(
    texture2d<float, access::write> outTL [[texture(0)]],
    texture2d<float, access::write> outTR [[texture(1)]],
    texture2d<float, access::write> outBL [[texture(2)]],
    texture2d<float, access::write> outBR [[texture(3)]],
    constant RockyConfig &config [[buffer(0)]],
    constant packed_float3* planetCP [[buffer(10)]],
    constant int32_t* planetKnotsU [[buffer(11)]],
    constant int32_t* planetKnotsV [[buffer(12)]],
    uint2 gid [[thread_position_in_grid]]
) {
    if (gid.x >= outTL.get_width() || gid.y >= outTL.get_height()) {
        return;
    }

    float u = ((float)gid.x / ((float)(outTL.get_width()))) * 0.5 + (float)config.quadrantX * 0.5;
    float v = ((float)gid.y / (float)(outTL.get_height() - 1)) * 0.5 + (float)config.quadrantY * 0.5;

    float3 p_ico = fromUVToPos(float2(u, v));
    
    float3 p_bspline;
    float3 n_eff;
    if (config.nCP_U > 0 && config.nCP_V > 0) {
        p_bspline = bspline_evaluate_local(u, v, planetCP, planetKnotsU, planetKnotsV, config);
        n_eff = normalize(p_bspline);
    } else {
        p_bspline = p_ico * config.planetRadius;
        n_eff = p_ico;
    }

    float3 p = p_bspline * config.fractalScale;
    
    float val;
    if (config.useTriplanar)
        val = ridgedFBMTriplanar_local(p, n_eff, config.fractalOctaves, config.useSimplexNoise);
    else
        val = ridgedFBM_local(p, config.fractalOctaves);

    if (config.quadrantX == 0 && config.quadrantY == 0) outTL.write(float4(val, 0.0, 0.0, 1.0), gid);
    if (config.quadrantX == 1 && config.quadrantY == 0) outTR.write(float4(val, 0.0, 0.0, 1.0), gid);
    if (config.quadrantX == 0 && config.quadrantY == 1) outBL.write(float4(val, 0.0, 0.0, 1.0), gid);
    if (config.quadrantX == 1 && config.quadrantY == 1) outBR.write(float4(val, 0.0, 0.0, 1.0), gid);
}
