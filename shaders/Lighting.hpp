//
// Created by Giovanni Bollati on 18/02/26.
//

#ifndef EVOLVING_PLANETS_LIGHTING_HPP
#define EVOLVING_PLANETS_LIGHTING_HPP

#include <metal_stdlib>
#include <metal_geometric>
#include <metal_matrix>
#include <bvh.hpp>

using namespace metal;

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct VertexPCNUV {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
};

// 32 byte
struct DirectionalLight {
    float3 direction;
    float4 color;
};
// 32 byte
struct PointLight {
    float3 position;
    float4 color;
};

// 16 + 32 * num + 32 * num
struct Lights {
    float4 globalAmbientLightColor;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    int numDirectionalLights;
    int _padding[3];
    PointLight pointLights[MAX_POINT_LIGHTS];
    int numPointLights;
    int _padding2[3];
};

static inline float4 applyPHONGLights(
                        thread float4 position,
                        thread float4 color,
                        thread float4 normal,
                        constant float& shininess,
                        constant float4& cameraPosition,
                        constant Lights& lights
                   ) {
    float4 cameraView = normalize(cameraPosition - position);
    
    float4 baseColor = color;

    // AMBIENT
    color = baseColor * lights.globalAmbientLightColor;

    // DIRECTIONAL
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        float3 C = lights.directionalLights[i].color.rgb;
        //if (length(C) < 0.01) continue;
        float3 L = -normalize(lights.directionalLights[i].direction);
        float3 V = cameraView.xyz;
        float3 N = normal.xyz;
        float dotNL = dot(L, N);

        // DIFFUSE
        float3 diffuse = max(0.0f, dotNL) * C * baseColor.rgb;
        color.rgb += diffuse;

        // SPECULAR
        if (dotNL <= 0) continue;
        float3 R = normalize(reflect(-L, N));
        color.rgb += C * pow(max(0.0f, dot(R, V)), shininess) * dotNL;

    }
    // POINT LIGHTS
    for (int i = 0; i < lights.numPointLights; ++i) {
        float3 C = lights.pointLights[i].color.rgb;
        //if (length(C) < 0.01f) continue;
        float3 lightVector = lights.pointLights[i].position - position.xyz;
        float3 V = cameraView.xyz;
        float distance = length(lightVector);
        float3 L = normalize(lightVector);
        float3 N = normal.xyz;
        float3 CD = C / (distance * distance);
        float dotNL = dot(N, L);

        // DIFFUSE
        float3 diffuse = max(0.0f, dotNL) * CD * baseColor.rgb;
        color.rgb += diffuse;

        // SPECULAR
        if (dotNL <= 0) continue;
        float3 R = normalize(reflect(-L, N));
        color.rgb += CD * pow(max(0.0f, dot(R, V)), shininess) * dotNL;
    }
    color.a = baseColor.a;
    return clamp(color, 0.0f, 1.0f);
}

static inline float4 applyWARDLights(
                        thread float4 worldPosition,
                        thread float4 color,
                        thread float4 normal,
                        constant float& roughness,
                        constant float& metallic,
                        constant float4& cameraPosition,
                        constant Lights& lights
                   ) {
    float3 baseColor = color.xyz;
    
    float3 rho_d = baseColor * (1.0f - metallic);
    float3 rho_s = mix(float3(0.04f), baseColor, metallic);

    // AMBIENT
    color.rgb *= lights.globalAmbientLightColor.rgb;
    
    // roughness
    float alpha = roughness;
    float alpha2 = alpha * alpha;

    float3 V = normalize(cameraPosition - worldPosition).xyz;
    float3 N = normalize(normal.xyz);
    float dotNV = dot(N, V);

    float3 C;

    // DIFFUSE
    // DIRECTIONAL
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        C = lights.directionalLights[i].color.rgb;
        //if (length(C) < 0.01) continue;
        float3 L = -normalize(lights.directionalLights[i].direction);
        float dotNL = dot(L, N);
        if (dotNL <= 0) continue;
        
        // DIFFUSE
        color.rgb += (rho_d / 3.14f) * C * dotNL;
        
        // SPECULAR
        if (dotNV <= 0) continue;
        float3 H = normalize(L + V); // half vector
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));
        
        color.rgb += C * D * rho_s * dotNL;
    }
    // POINT LIGHTS
    for (int i = 0; i < lights.numPointLights; ++i) {
        C = lights.pointLights[i].color.rgb;
        //if (length(C) < 0.01f) continue;
        
        float3 LnotNorm = lights.pointLights[i].position - worldPosition.xyz;
        float distance = length(LnotNorm);
        float3 L = normalize(LnotNorm);
        float3 CD = C / (distance * distance);
        float dotNL = dot(N, L);
        if (dotNL <= 0) continue;
        
        // DIFFUSE
        color.rgb += CD * dotNL;
        
        // SPECULAR
        if (dotNV <= 0) continue;
        float3 H = normalize(L + V); // Vettore somma (non normalizzato nella tua formula)
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        // Esempio semplificato della struttura della formula
        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        //if (dotNL <= 0) continue;
        color.rgb += CD * D * rho_s * dotNL;
    }
    return clamp(color, 0.0f, 1.0f);
}

struct ShadowData {
    float4x4 viewProjectionMatrix[MAX_DIRECTIONAL_LIGHTS];
};

struct PointShadowData {
    float4x4 matrices[MAX_POINT_LIGHTS * 6];
    float4 positions[MAX_POINT_LIGHTS];
    float4 farPlanes[MAX_POINT_LIGHTS];
};

static inline float4 applySHADOWWARDLights(
                        thread float4 worldPosition,
                        thread float4 color,
                        thread float4 normal,
                        constant float& roughness,
                        constant float& metallic,
                        constant float4& cameraPosition,
                        constant Lights& lights,
                        constant ShadowData& shadowData,
                        depth2d_array<float> shadowMaps,
                        sampler textureSampler
                   ) {
    float3 baseColor = color.xyz;
    
    float3 rho_d = baseColor * (1.0f - metallic);
    float3 rho_s = mix(float3(0.04f), baseColor, metallic);

    // AMBIENT
    color.rgb *= lights.globalAmbientLightColor.rgb;
    
    // roughness
    float alpha = roughness;
    float alpha2 = alpha * alpha;

    float3 V = normalize(cameraPosition - worldPosition).xyz;
    float3 N = normalize(normal.xyz);
    float dotNV = dot(N, V);

    float3 C;

    // DIFFUSE
    // DIRECTIONAL
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        C = lights.directionalLights[i].color.rgb;
        //if (length(C) < 0.01) continue;
        if (dotNV <= 0) continue;
        float3 L = -normalize(lights.directionalLights[i].direction);
        float dotNL = dot(L, N);
        if (dotNL <= 0) continue;
        
        // APPLY SHADOW
        float4 lightSpacePos = shadowData.viewProjectionMatrix[i] * worldPosition;
        float3 ndc = lightSpacePos.xyz / lightSpacePos.w;

        float2 shadowUV;
        shadowUV.x =  ndc.x * 0.5 + 0.5;   // [-1,+1] → [0,1]
        shadowUV.y = -ndc.y * 0.5 + 0.5;   // [-1,+1] → [0,1] con Y invertita
        
        float currentDepth = ndc.z;
        
        /*
        if (!(shadowUV.x <= 0.0 || shadowUV.x >= 1.0 ||
            shadowUV.y <= 0.0 || shadowUV.y >= 1.0 ||
              currentDepth < 0.0 || currentDepth > 1.0)) {
            float shadowMapDepth = shadowMaps.sample(textureSampler, shadowUV, i);
            if (currentDepth > shadowMapDepth + 0.0001f) continue;
        }*/
        
        if (shadowUV.x > 0.0 and shadowUV.x < 1.0 and
            shadowUV.y > 0.0 and shadowUV.y < 1.0 and
              currentDepth > 0.0 and currentDepth < 1.0) {
            float shadowMapDepth = shadowMaps.sample(textureSampler, shadowUV, i);
            if (currentDepth > shadowMapDepth + 0.001f) continue;
        }
        
        // DIFFUSE
        color.rgb += (rho_d / 3.14f) * C * dotNL;
        
        // SPECULAR
        float3 H = normalize(L + V); // half vector
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        color.rgb += C * D * rho_s * dotNL;
    }
    // POINT LIGHTS
    for (int i = 0; i < lights.numPointLights; ++i) {
        C = lights.pointLights[i].color.rgb;
        //if (length(C) < 0.01f) continue;
        if (dotNV <= 0) continue;
        
        float3 LnotNorm = lights.pointLights[i].position - worldPosition.xyz;
        float distance = length(LnotNorm);
        float3 L = normalize(LnotNorm);
        float3 CD = C / (distance * distance);
        float dotNL = dot(N, L);
        
        if (dotNL <= 0) continue;
        
        // DIFFUSE
        color.rgb += CD * dotNL;
        
        // SPECULAR
        float3 H = normalize(L + V); // Vettore somma (non normalizzato nella tua formula)
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        // Esempio semplificato della struttura della formula
        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        //if (dotNL <= 0) continue;
        color.rgb += CD * D * rho_s * dotNL;
    }
    return clamp(color, 0.0f, 1.0f);
}

static inline float4 applyFULLSHADOWWARDLights(
                        thread float4 worldPosition,
                        thread float4 color,
                        thread float4 normal,
                        constant float& roughness,
                        constant float& metallic,
                        constant float4& cameraPosition,
                        constant Lights& lights,
                        constant ShadowData& shadowData,
                        constant PointShadowData& pointShadowData,
                        depth2d_array<float> shadowMaps,
                        depthcube_array<float> cubeMaps,
                        sampler textureSampler
                   ) {
    float3 baseColor = color.xyz;
    
    float3 rho_d = baseColor * (1.0f - metallic);
    float3 rho_s = mix(float3(0.04f), baseColor, metallic);

    // AMBIENT
    color.rgb *= lights.globalAmbientLightColor.rgb;
    
    // roughness
    float alpha = roughness;
    float alpha2 = alpha * alpha;

    float3 V = normalize(cameraPosition - worldPosition).xyz;
    float3 N = normalize(normal.xyz);
    float dotNV = dot(N, V);

    float3 C;

    // DIFFUSE
    // DIRECTIONAL
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        C = lights.directionalLights[i].color.rgb;
        //if (length(C) < 0.01) continue;
        if (dotNV <= 0) continue;
        float3 L = -normalize(lights.directionalLights[i].direction);
        float dotNL = dot(L, N);
        if (dotNL <= 0) continue;
        
        // APPLY SHADOW
        float4 lightSpacePos = shadowData.viewProjectionMatrix[i] * worldPosition;
        float3 ndc = lightSpacePos.xyz / lightSpacePos.w;

        float2 shadowUV;
        shadowUV.x =  ndc.x * 0.5 + 0.5;   // [-1,+1] → [0,1]
        shadowUV.y = -ndc.y * 0.5 + 0.5;   // [-1,+1] → [0,1] con Y invertita
        
        float currentDepth = ndc.z;
    
        float shadowFactor = 1.0f;
        if (shadowUV.x > 0.0 and shadowUV.x < 1.0 and
            shadowUV.y > 0.0 and shadowUV.y < 1.0 and
              currentDepth > 0.0 and currentDepth < 1.0) {
            /*
            float shadowMapDepth = shadowMaps.sample(textureSampler, shadowUV, i);
            if (currentDepth > shadowMapDepth + 0.001f) continue;
             */
            float shadowAccum = 0.0;
                
            // Calcola la dimensione di un singolo texel nella shadow map
            // Se la tua texture è 1024x1024, il texel è 1.0/1024.0
            float texelSize = 1.0 / 1024.0;

                // Loop PCF 3x3
            for(int x = -1; x <= 1; ++x) {
                for(int y = -1; y <= 1; ++y) {
                    float2 offset = float2(x, y) * texelSize;
                    float pcfDepth = shadowMaps.sample(textureSampler, shadowUV + offset, i);
                    shadowAccum += (currentDepth > pcfDepth + 0.001f) ? 0.0 : 1.0;
                }
            }
                
            // Fai la media dei 9 campionamenti
            shadowFactor = shadowAccum / 9.0;
        }
        
        // DIFFUSE
        color.rgb += (rho_d / 3.14f) * C * dotNL * shadowFactor;
        
        // SPECULAR
        float3 H = normalize(L + V); // half vector
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        color.rgb += C * D * rho_s * dotNL * shadowFactor;
    }
    // POINT LIGHTS
    for (int i = 0; i < lights.numPointLights; ++i) {
        C = lights.pointLights[i].color.rgb;
        //if (length(C) < 0.01f) continue;
        if (dotNV <= 0) continue;
        
        float3 LnotNorm = lights.pointLights[i].position - worldPosition.xyz;
        
        auto cubeMapDistance = cubeMaps.sample(textureSampler, -LnotNorm, i);
        float distance = length(LnotNorm);
        auto farDistance = distance / pointShadowData.farPlanes[i].x;
        
        /*
        if (farDistance > cubeMapDistance + 0.005f) {
            continue;
        }
         */
        float3 offsets[8] = {
            float3( 1,  1,  1), float3( 1, -1,  1), float3(-1, -1,  1), float3(-1,  1,  1),
            float3( 1,  1, -1), float3( 1, -1, -1), float3(-1, -1, -1), float3(-1,  1, -1)
        };
        float diskRadius = 0.005; // Regola questo valore per la morbidezza

        float shadowAccum = 0.0;
        float3 sampleDir = -LnotNorm;

        for (int j = 0; j < 8; j++) {
            // Campiona la cubemap con un leggero offset
            float pcfDepth = cubeMaps.sample(textureSampler, sampleDir + offsets[j] * diskRadius, i);
            
            // Usa il bias che abbiamo trovato (0.005)
            shadowAccum += (farDistance > pcfDepth + 0.005f) ? 0.0 : 1.0;
        }

        // 3. Media dei risultati
        float shadowFactor = shadowAccum / 8.0;
        
        float3 L = normalize(LnotNorm);
        float3 CD = C / (distance * distance);
        float dotNL = dot(N, L);
        
        if (dotNL <= 0) continue;
        
        // DIFFUSE
        color.rgb += CD * dotNL * shadowFactor;
        
        // SPECULAR
        float3 H = normalize(L + V); // Vettore somma (non normalizzato nella tua formula)
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        // Esempio semplificato della struttura della formula
        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        //if (dotNL <= 0) continue;
        color.rgb += CD * D * rho_s * dotNL * shadowFactor;
    }
    return clamp(color, 0.0f, 1.0f);
}

static inline float3 calculateOrenNayarDiffuse(float3 N, float3 L, float3 V, float roughness, float3 albedo) {
    float dotNL = dot(N, L);
    float dotNV = dot(N, V);
    
    // Se la luce è dietro la superficie, non c'è illuminazione
    if (dotNL <= 0.0f) return float3(0.0f);
    
    // Calcoliamo i parametri A e B basati sulla rugosità della polvere
    float sigma2 = roughness * roughness;
    float A = 1.0f - 0.5f * (sigma2 / (sigma2 + 0.33f));
    float B = 0.45f * (sigma2 / (sigma2 + 0.09f));
    
    // Calcolo degli angoli di sfericità (Theta e Phi)
    float theta_i = acos(clamp(dotNL, -1.0f, 1.0f));
    float theta_r = acos(clamp(dotNV, -1.0f, 1.0f));
    
    float alpha = max(theta_i, theta_r);
    float beta  = min(theta_i, theta_r);
    
    // Calcolo della differenza dell'azimut (proiezione dei vettori sul piano tangente)
    float3 light_tangent = normalize(L - N * dotNL);
    float3 view_tangent  = normalize(V - N * dotNV);
    float cos_phi_diff   = max(0.0f, dot(light_tangent, view_tangent));
    
    // Formula finale Oren-Nayar
    float diffuse_factor = dotNL * (A + B * cos_phi_diff * sin(alpha) * tan(beta));
    
    // 1 / PI è il fattore di normalizzazione energetica
    return albedo * (diffuse_factor * (1.0f / 3.14159265f));
}


static inline float4 applyFULLSHADOWWARDLightsWithOrenNayar(
                        thread float4 worldPosition,
                        thread float4 color,
                        thread float4 normal,
                        constant float& roughness,
                        constant float& metallic,
                        constant float4& cameraPosition,
                        constant Lights& lights,
                        constant ShadowData& shadowData,
                        constant PointShadowData& pointShadowData,
                        depth2d_array<float> shadowMaps,
                        depthcube_array<float> cubeMaps,
                        sampler textureSampler,
                        float hbao,
                        constant BVHNode* bvhNodes,
                        constant Triangle* bvhPrimitives,
                        int useRayTracedShadow
                   ) {
    float3 baseColor = color.xyz;
    
    // rho_diffuse
    float3 rho_d = baseColor * (1.0f - metallic);
    // rho_specular
    float3 rho_s = mix(float3(0.04f), baseColor, metallic);

    // AMBIENT
    color.rgb *= lights.globalAmbientLightColor.rgb;
    
    // roughness
    float alpha = roughness;
    float alpha2 = alpha * alpha;

    // V: worldPosition -> camera
    float3 V = normalize(cameraPosition - worldPosition).xyz;
    float3 N = normalize(normal.xyz);
    float dotNV = dot(N, V);

    float3 C;

    // DIFFUSE
    // DIRECTIONAL
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        C = lights.directionalLights[i].color.rgb;
        //if (length(C) < 0.01) continue;
        if (dotNV <= 0) continue;
        float3 L = -normalize(lights.directionalLights[i].direction);
        float dotNL = dot(L, N);
        if (dotNL <= 0) continue;
        
        // APPLY FORCE SHADOW
        if (useRayTracedShadow > 0 and intersect((worldPosition + normal * 1.0).xyz, L, bvhNodes, bvhPrimitives)) continue;
        
        //if (hbao < 0.96) break;
        
        // APPLY SHADOW
        // Sposta la posizione del frammento leggermente verso l'esterno lungo la normale della superficie
        // per "sollevare" matematicamente il calcolo sopra lo spessore del gradino della shadow map
        float3 biasedWorldPos = worldPosition.xyz + normal.xyz * 1.0f;

        // Ricalcola le coordinate di campionamento shadowUV e la currentDepth usando biasedWorldPos...
        float4 lightSpacePos = shadowData.viewProjectionMatrix[i] * float4(biasedWorldPos, 1.0);
        float3 ndc = lightSpacePos.xyz / lightSpacePos.w;

        float2 shadowUV;
        shadowUV.x =  ndc.x * 0.5 + 0.5;   // [-1,+1] → [0,1]
        shadowUV.y = -ndc.y * 0.5 + 0.5;   // [-1,+1] → [0,1] con Y invertita
        
        float currentDepth = ndc.z;
    
        float shadowFactor = 1.0f;
        
        bool usePCF = true;
        // Bias dinamico basato sull'inclinazione per evitare shadow acne
        float bias = 0.002f * clamp(1.0 - dotNL, 0.0, 1.0);
        
        if (shadowUV.x > 0.0 and shadowUV.x < 1.0 and
            shadowUV.y > 0.0 and shadowUV.y < 1.0 and
            currentDepth > 0.0 and currentDepth < 1.0) {
        
            
            if (usePCF) {
                float shadowAccum = 0.0;
                
                // Calcola la dimensione di un singolo texel nella shadow map
                // Se la tua texture è 1024x1024, il texel è 1.0/1024.0
                float texelSize = 1.0 / 4096.0;
                
                // Loop PCF 3x3
                for(int x = -1; x <= 1; ++x) {
                    for(int y = -1; y <= 1; ++y) {
                        float2 offset = float2(x, y) * texelSize;
                        float pcfDepth = shadowMaps.sample(textureSampler, shadowUV + offset, i);
                        shadowAccum += (currentDepth > pcfDepth + bias) ? 0.0 : 1.0;
                    }
                }
                
                // Fai la media dei 9 campionamenti
                shadowFactor = shadowAccum / 9.0;
            } else {
                
                float shadowMapDepth = shadowMaps.sample(textureSampler, shadowUV, i);
                if (currentDepth > shadowMapDepth + bias) {
                    continue;
                }
            }
        }
        
        // DIFFUSE
        color.rgb += (rho_d / 3.14f) * C * dotNL * shadowFactor;
        
        // SPECULAR
        float3 H = normalize(L + V); // half vector
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        color.rgb += C * D * rho_s * dotNL * shadowFactor;
    }
    // POINT LIGHTS
    for (int i = 0; i < lights.numPointLights; ++i) {
        C = lights.pointLights[i].color.rgb;
        //if (length(C) < 0.01f) continue;
        if (dotNV <= 0) continue;
        
        float3 LnotNorm = lights.pointLights[i].position - worldPosition.xyz;
        
        auto cubeMapDistance = cubeMaps.sample(textureSampler, -LnotNorm, i);
        float distance = length(LnotNorm);
        auto farDistance = distance / pointShadowData.farPlanes[i].x;
        
        /*
        if (farDistance > cubeMapDistance + 0.005f) {
            continue;
        }
         */
        float3 offsets[8] = {
            float3( 1,  1,  1), float3( 1, -1,  1), float3(-1, -1,  1), float3(-1,  1,  1),
            float3( 1,  1, -1), float3( 1, -1, -1), float3(-1, -1, -1), float3(-1,  1, -1)
        };
        float diskRadius = 0.005; // Regola questo valore per la morbidezza

        float shadowAccum = 0.0;
        float3 sampleDir = -LnotNorm;

        for (int j = 0; j < 8; j++) {
            // Campiona la cubemap con un leggero offset
            float pcfDepth = cubeMaps.sample(textureSampler, sampleDir + offsets[j] * diskRadius, i);
            
            // Usa il bias che abbiamo trovato (0.005)
            if (farDistance <= 1.0f) {
                shadowAccum += (farDistance > pcfDepth + 0.005f) ? 0.0 : 1.0;
            } else {
                shadowAccum += 1.0;
            }
        
        }

        // 3. Media dei risultati
        float shadowFactor = shadowAccum / 8.0;
        
        float3 L = normalize(LnotNorm);
        float3 CD = C / (distance * distance);
        float dotNL = dot(N, L);
        
        if (dotNL <= 0) continue;
        
        // DIFFUSE
        color.rgb += CD * dotNL * shadowFactor;
        
        // SPECULAR
        float3 H = normalize(L + V); // Vettore somma (non normalizzato nella tua formula)
        float dotNH = dot(H, N);
        float dotNH2 = dotNH * dotNH;
        float tan2ThetaH = (1.0 - dotNH2) / dotNH2;

        // Esempio semplificato della struttura della formula
        float exponent = -tan2ThetaH / alpha2;
        float D = exp(exponent) / (4.0f * 3.1415 * alpha2 * sqrt(dotNL * dotNV));

        //if (dotNL <= 0) continue;
        color.rgb += CD * D * rho_s * dotNL * shadowFactor;
    }
    return color;
}
#endif //EVOLVING_PLANETS_LIGHTING_HPP
