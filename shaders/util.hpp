//
// Created by Giovanni Bollati on 16/04/26.
//

#ifndef EVOLVING_PLANETS_UTIL_HPP
#define EVOLVING_PLANETS_UTIL_HPP

#include <metal_stdlib>
using namespace metal;

inline float3 hash3d_point(float3 p) {
    p = fract(p * float3(0.1031, 0.1030, 0.0973));
    p += dot(p, p.yxz + 33.33);
    return fract((p.xxy + p.yxx) * p.zyx);
}

inline float hash1d(float3 p) {
    float3 p3  = fract(p * float3(.1031, .1030, .0973));
    p3 += dot(p3, p3.yzx + 33.33);
    return fract((p3.x + p3.y) * p3.z);
}

inline float smoothNoise(float3 p) {
    float3 i = floor(p);
    float3 f = fract(p);
    
    f = f * f * (float3(3.0) - 2.0 * f); // Interpolazione Hermite

    // Campioniamo gli 8 angoli del cubo
    float a = hash1d(i + float3(0,0,0));
    float b = hash1d(i + float3(1,0,0));
    float c = hash1d(i + float3(0,1,0));
    float d = hash1d(i + float3(1,1,0));
    float e = hash1d(i + float3(0,0,1));
    float g = hash1d(i + float3(1,0,1));
    float h = hash1d(i + float3(0,1,1));
    float j = hash1d(i + float3(1,1,1));

    return mix(mix(mix(a, b, f.x), mix(c, d, f.x), f.y),
               mix(mix(e, g, f.x), mix(h, j, f.x), f.y), f.z);
}

inline float fbm(float3 p) {
    float v = 0.0;
    float a = 0.5;
    for (int i = 0; i < 10; i++) {
        v += smoothNoise(p) * a;
        p *= 2.0;
        a *= 0.5;
    }
    return v;
}

inline float2 fromPosToUV(float3 p) {
    float phi = atan2(-p.x, p.z);
    float theta = acos(clamp(p.y, -1.0f, 1.0f));
    float u_planet = (phi + M_PI_F) / (2.0 * M_PI_F);
    float v_planet = clamp(theta / M_PI_F, 0.0, 1.0);
    return float2(u_planet, v_planet);
}

inline float3 fromUVToPos(float2 uv) {
    float phi = (uv.x * 2.0 * M_PI_F) - M_PI_F;
    float theta = uv.y * M_PI_F;

    float x = -sin(phi) * sin(theta);
    float y = cos(theta);
    float z = cos(phi) * sin(theta);

    return float3(x, y, z);
}

inline float4 getRockyPlanetColor(float3 worldPos, float3 normal) {
    // 1. Direzione radiale e pendenza
    float3 up = normalize(worldPos);
    float slope = dot(normal, up);
    
    // 2. Palette Minerale
    float3 darkRock   = float3(0.15, 0.14, 0.13); // Basalto / Roccia vulcanica
    float3 dustColor  = float3(0.45, 0.42, 0.38); // Regolite / Polvere chiara
    float3 highlight  = float3(0.60, 0.58, 0.55); // Picchi minerali esposti
    
    // 3. Calcolo dell'accumulo di polvere (Regolite)
    // La polvere si deposita dove la superficie è piana (slope vicino a 1.0)
    float dustAccumulation = smoothstep(0.4, 0.9, slope);
    
    // 4. Variazione basata sull'altitudine per simulare strati geologici
    // Usiamo il 'sin' per creare leggere variazioni di colore a diverse altezze
    float altitudeLayer = sin(length(worldPos) * 0.5) * 0.05;
    
    // 5. Composizione finale
    float3 baseColor = mix(darkRock, dustColor, dustAccumulation);
    
    // Aggiungiamo un tocco di "highlight" sulle creste più alte e piatte
    float highAltitude = smoothstep(1010.0, 1050.0, length(worldPos));
    baseColor = mix(baseColor, highlight, highAltitude * dustAccumulation);

    // Applichiamo la variazione geologica
    baseColor += altitudeLayer;

    return float4(baseColor, 1.0);
}

inline float ridgedFBM(float3 p, int octaves) {
    float value = 0.0f;
    float amplitude = 0.5f;
    float frequency = 3.0f;
    float weight = 1.0f;

    for (int i = 0; i < octaves; i++) {
        float n = smoothNoise(p * frequency);
        
        n = 1.0f - abs(n);
        n *= n;

        value += n * amplitude * weight;
        weight = n;

        frequency *= 2.0f;
        amplitude *= 0.5f;
    }
    return value;
}

#endif //EVOLVING_PLANETS_UTIL_HPP
