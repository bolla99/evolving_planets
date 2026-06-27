#include <metal_stdlib>
#include <metal_geometric>
#include <metal_matrix>

using namespace metal;

#include "Lighting.hpp"
#include "util.hpp"

struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float4 normal;
    float4 color;
    float2 uv;
    float3 localPosition;
    float3 instanceWorldPosition [[flat]];
    float4 currentClipPosition;
    float4 previousClipPosition;
};

vertex VertexOut vertexFlame(
    VertexPCNUV in [[stage_in]],
    constant float4x4& previousModelMatrix [[buffer(25)]],
    constant float3x3& normalMatrix [[buffer(26)]],
    constant float4x4& modelMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(29)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(30)]]
) {
    VertexOut out;
    // position
    out.worldPosition = (modelMatrix * float4(in.position, 1.0f));
    out.position = jitteredViewProjectionMatrix * out.worldPosition;
    out.color = in.color;
    out.normal = normalize(float4(float3x3(normalMatrix) * in.normal, 0.0f));
    out.uv = in.uv;
    out.instanceWorldPosition = modelMatrix.columns[3].xyz;
    out.localPosition = in.position;

    out.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(in.position, 1.0);
    out.previousClipPosition = previousViewProjectionMatrix * previousModelMatrix * float4(in.position, 1.0);
    
    return out;
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut fragmentFlame(
        VertexOut in [[stage_in]],
        bool frontFacing [[front_facing]],
        constant float& alpha [[buffer(22)]],
        constant ShadowData& shadowData [[buffer(23)]],
        constant PointShadowData& pointShadowData [[buffer(24)]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        constant float& time [[buffer(29)]],
        constant float& thurst [[buffer(30)]],
        depth2d_array<float> shadowMaps [[texture(0)]],
        depthcube_array<float> cubeMaps [[texture(1)]],
        sampler textureSampler [[sampler(0)]]
) {
    auto V = normalize(cameraPosition - in.worldPosition);
    auto N = normalize(in.normal);
    if (not frontFacing) N = -N;
    
    auto fresnel = 1.0f - saturate(dot(V, N));
    fresnel *= fresnel;
    
    float fade = smoothstep(thurst, 0.0f, in.uv.y);
    
    auto noiseSpeed = thurst * 50.0f;
    float3 coordRumore = float3(50.0f + 50.0f * in.uv.x, 20.0f * in.uv.y - time * noiseSpeed, 10.0f);
    auto noise = smoothNoise(coordRumore);
    
    float fireDensity = fresnel * fade * (noise);
    

    float diamondsSpeed = thurst * 50.0f;
    float shockDiamonds = sin(in.uv.y * 30.0f - time * diamondsSpeed) * 0.5f + 0.5f;
    shockDiamonds *= smoothstep(0.8f * thurst, 0.2f * thurst, in.uv.y);
    fireDensity += shockDiamonds * 0.3f; // Aggiunge piccoli picchi di luce ritmici
    
    fireDensity = saturate(fireDensity);
    
    float3 coloreCore = float3(2.0f, 2.0f, 2.0f);
    float3 coloreFiamma = float3(0.0f, 0.8f, 2.0f);
    float4 finalColor = float4(mix(coloreFiamma, coloreCore, pow(fireDensity, 2.0f)), fireDensity);

    /*
    auto finalColor = applyFULLSHADOWWARDLightsWithOrenNayar(
                                {in.color, in.normal, in.worldPosition},
                                 roughness, metallic,
                                 cameraPosition, lights, float3(1.0),
                                 shadowData,
                                 pointShadowData,
                                 shadowMaps,
                                 cubeMaps,
                                 textureSampler, nullptr, nullptr, 0, 0.0f, 0.01f, false
                                 );
     */
            
    
    
    // DITHERING
    float dist = distance(in.instanceWorldPosition, cameraPosition.xyz);
    auto fadeNearDistance = 2.0f;
    auto fadeFarDistance = 5.0f;
    float a = smoothstep(fadeNearDistance, fadeFarDistance, dist);
    
    float threshold = interleavedGradientNoise(in.position.xy);
    if (a < threshold) {
        discard_fragment();
    }
    
    FragmentOut out;
    out.color = finalColor;
    out.motionVector = motionVector(in.currentClipPosition, in.previousClipPosition);
    return out;
}


