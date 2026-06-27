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

vertex VertexOut vertexWARD(
    VertexPCNUV vertexIn [[stage_in]],
    constant float3x3& normalMatrix [[buffer(27)]],
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    // position
    vertexOut.worldPosition = (modelMatrix * float4(vertexIn.position, 1.0f));
    vertexOut.position = projectionMatrix * viewMatrix * vertexOut.worldPosition;
    vertexOut.color = vertexIn.color;
    vertexOut.normal = normalize(float4(float3x3(normalMatrix) * normalize(vertexIn.normal), 0.0f));
    vertexOut.uv = vertexIn.uv;
    
    return vertexOut;
}

fragment float4 fragmentVCWARD(
        VertexOut vertexOut [[stage_in]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]]
) {
    return applyWARDLights(vertexOut.worldPosition, vertexOut.color, vertexOut.normal, roughness, metallic, cameraPosition, lights);
}


fragment float4 fragmentTextureWARD(
        VertexOut vertexOut [[stage_in]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        texture2d<float> diffuseTexture [[texture(0)]],
        sampler textureSampler [[sampler(0)]]
) {
    float4 position = vertexOut.worldPosition;
    float4 texColor = diffuseTexture.sample(textureSampler, vertexOut.uv);
    float4 normal = normalize(vertexOut.normal);
    
    return applyWARDLights(position, texColor, normal, roughness, metallic, cameraPosition, lights);
}


float3 rockyfyPosition(float3 position, float3 normal) {
    return position.xyz + fbm(position.xyz) * normal.xyz * 0.2;
}
float3 rockyfyNormal(float3 p, float3 normal) {
    float epsilon = 0.01; // Un passo piccolissimo

    // Campioniamo l'altezza (fbm) in 3 direzioni
    float h = fbm(p);
    float hx = fbm(p + float3(epsilon, 0, 0));
    float hy = fbm(p + float3(0, epsilon, 0));
    float hz = fbm(p + float3(0, 0, epsilon));

    // Calcoliamo la pendenza (gradiente)
    float3 gradient = float3(h - hx, h - hy, h - hz) / epsilon;

    // La nuova normale è la vecchia normale "distorta" dal gradiente
    return normalize(normal + gradient * 0.5);
}

vertex VertexOut vertexWARDSHADOW(
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

fragment float4 fragmentVCWARDSHADOW(
        VertexOut vertexOut [[stage_in]],
        constant ShadowData& shadowData [[buffer(24)]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        depth2d_array<float> shadowMaps [[texture(0)]],
        sampler textureSampler [[sampler(0)]]
) {
    return applySHADOWWARDLights(
                                 vertexOut.worldPosition,
                                 vertexOut.color,
                                 vertexOut.normal,
                                 roughness, metallic,
                                 cameraPosition, lights,
                                 shadowData,
                                 shadowMaps,
                                 textureSampler
                                 );
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut fragmentVCWARDFULLSHADOW(
        VertexOut in [[stage_in]],
        constant float& alpha [[buffer(22)]],
        constant ShadowData& shadowData [[buffer(23)]],
        constant PointShadowData& pointShadowData [[buffer(24)]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        depth2d_array<float> shadowMaps [[texture(0)]],
        depthcube_array<float> cubeMaps [[texture(1)]],
        sampler textureSampler [[sampler(0)]]
) {
    
    float3 finalAlbedo = in.color.xyz;
    float finalRoughness = roughness;
    float finalMetallic = metallic;
    
    auto c = applyFULLSHADOWWARDLightsWithOrenNayar(
                                {float4(finalAlbedo, 1.0), in.normal, in.worldPosition},
                                 finalRoughness, finalMetallic,
                                 cameraPosition, lights, float3(1.0),
                                 shadowData,
                                 pointShadowData,
                                 shadowMaps,
                                 cubeMaps,
                                 textureSampler, nullptr, nullptr, 0, 0.0f, 0.001f, false
                                 );
            
    // DITHERING
    float dist = distance(in.instanceWorldPosition, cameraPosition.xyz);
    auto fadeNearDistance = 3.0f;
    auto fadeFarDistance = 10.0f;
    float a = smoothstep(fadeNearDistance, fadeFarDistance, dist);

    float threshold = interleavedGradientNoise(in.position.xy);
    if (a < threshold) {
        discard_fragment();
    }
    
    FragmentOut out;
    out.color = float4(c.rgb, c.a * alpha);
    out.motionVector = motionVector(in.currentClipPosition, in.previousClipPosition);
    return out;
}



