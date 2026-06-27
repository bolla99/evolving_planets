//
// Created by Giovanni Bollati on 15/06/26.
//

#ifndef EVOLVING_PLANETS_RAYLEIGH_HPP
#define EVOLVING_PLANETS_RAYLEIGH_HPP

#include "Lighting.hpp"


struct PotentialSamplingInfo {
    float4 min;
    float edge;
    float _padding[3];
    float nonZeroDensityRadius;
    float _padding2[3];
};

struct AtmosphereSettings
{
    int SAMPLES;
    int _padding[3];
    int SUN_SAMPLES;
    int _padding2[3];
    bool jitter;
    int _padding3[3];
    bool useBakedLightTransmittance;
};

// 1. FUNZIONE DI INTERSEZIONE RAGGIO-SFERA STANDARD
inline bool raySphereIntersect(float3 ro, float3 rd, float3 center, float radius, thread float& t0, thread float& t1) {
    float3 oc = ro - center;
    float b = dot(oc, rd);
    float c = dot(oc, oc) - radius * radius;
    float h = b * b - c;
    if (h < 0.0f) return false; // Il raggio manca la sfera
    h = sqrt(h);
    t0 = -b - h;
    t1 = -b + h;
    return true;
}

// 2. FILTRAGGIO TRICUBICO/SMOOTH ADATTIVO PER TEXTURE 3D
// Questa funzione implementa il filtraggio C2-continuo a livello sub-voxel (Quilez warp).
// Trasforma le coordinate lineari prima del campionamento hardware trilineare per emulare
// un'interpolazione spline cubica (Smooth Super-sampling). Elimina completamente i bordi
// netti e l'aspetto "pixelato/voxetizzato" derivante dal campionamento trilineare standard.
inline float sampleDensitySmooth3D(texture3d<float> tex, sampler s, float3 uvw) {
    float3 size = float3(tex.get_width(), tex.get_height(), tex.get_depth());
    float3 texel = uvw * size - 0.5f;
    float3 f = fract(texel);
    float3 i = floor(texel);

    // Curva quintica di Perlin (C2-continua, elimina i gradini visibili tra voxel):
    float3 f_smooth = f * f * f * (f * (f * 6.0f - 15.0f) + 10.0f);

    float3 uvw_smooth = (i + f_smooth + 0.5f) / size;
    return tex.sample(s, uvw_smooth).x;
}

inline float3 lightTransmittance(
                                 float3 P, float3 L, constant PotentialSamplingInfo& info, float3 betaR, texture3d<float> densityTexture, sampler densitySampler, float jitter, depth2d_array<float> shadowMap, constant ShadowData& shadowData, int numLights, float3 normal, bool useBiasedP, float biasAmount, float shadowBias, bool shadowFrontCulling = false, int SUN_SAMPLES = 8
) {
    if (shadowFrontCulling) shadowBias = -shadowBias;
    float3 biasedP = P;
    if (useBiasedP) biasedP += biasAmount * normal;
    // Per la light transmittance dell'atmosfera valutiamo solo la mappa d'ombra globale (l'ultimo pass / il più generico)
    if (numLights > 0) {
        int i = numLights - 1;
        auto clip = shadowData.viewProjectionMatrix[i] * float4(biasedP, 1.0);
        clip /= clip.w;
        auto uv = float2(clip.x * 0.5 + 0.5, -clip.y * 0.5 + 0.5);
        auto depth = clip.z;
        if (uv.x > 0 and uv.x < 1 and uv.y > 0 and uv.y < 1 and depth > 0 and depth < 1) {
            auto bakedDepth = shadowMap.sample(densitySampler, uv, i);
            if (depth > (bakedDepth + shadowBias)) return float3(0.0);
        }
    }
    
    // Troviamo dove il raggio del sole esce dall'atmosfera globale
    float ts0 = 0.0;
    float ts1 = 0.0;
    
    raySphereIntersect(P, L, float3(0.0), info.nonZeroDensityRadius, ts0, ts1);
    
    float distanceToSpace = max(0.0f, ts1);
    float stepL = distanceToSpace / float(SUN_SAMPLES);
    
    float opticalDepth = 0.0f;
    
    for (int j = 0; j < SUN_SAMPLES; j++) {
        float3 samplePoint = P + L * (stepL * (float(j) + jitter));
        auto normalizedP = (samplePoint - info.min.xyz) / info.edge;
        float density = 0.0f;
        if (all(normalizedP >= 0.0f) && all(normalizedP <= 1.0f)) {
            density = sampleDensitySmooth3D(densityTexture, densitySampler, normalizedP);
        }

        opticalDepth += density * stepL;
    }
    
    return exp(-betaR * opticalDepth);
}

inline float3 lightTransmittanceFromTexture(
                                            float3 P,
                                            constant PotentialSamplingInfo& info,
                                            texture3d<float> opticalDepthTexture,
                                            sampler s, float3 betaR
                                            ) {
    auto normalizedP = (P - info.min.xyz) / info.edge;
    auto scalarLightTransmittance = opticalDepthTexture.sample(s, normalizedP).x;
    return pow(scalarLightTransmittance, betaR);
}

#endif //EVOLVING_PLANETS_RAYLEIGH_HPP
