#include <metal_stdlib>
using namespace metal;

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct VertexIn {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
    float2 uv [[attribute(3)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float2 uv;
};

struct DirectionalLight {
    float3 direction;
    float4 color;
};

struct PointLight {
    float3 position;
    float4 color;
};

struct Lights {
    float4 globalAmbientLightColor;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    int numDirectionalLights;
    int numPointLights;
    int _padding[2];
};



vertex VertexOut vertexTexturePHONG(
    VertexIn vertexIn [[stage_in]],
    constant Lights& lights [[buffer(28)]],
    constant float4x4& modelMatrix [[buffer(29)]],
    constant float4x4& viewProjectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = viewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = float4(0.0f, 0.0f, 0.0f, 1.0f);
    vertexOut.uv = vertexIn.uv;

    // Calculate lighting
    float3 normal = normalize(vertexIn.normal);
    // apply global ambient light
    vertexOut.color.rgb += lights.globalAmbientLightColor.rgb;
    // apply directional light
    for (int i = 0; i < lights.numDirectionalLights; ++i) {
        float3 directionalLightColor = max(0.0f, dot(normal, -lights.directionalLights[i].direction)) * lights.directionalLights[i].color.rgb;
        vertexOut.color.rgb += directionalLightColor;
    }
    // apply point lights
    for (int i = 0; i < lights.numPointLights; ++i) {
        float3 lightDir = normalize(lights.pointLights[i].position - vertexIn.position);
        float3 pointLightColor = max(0.0f, dot(normal, lightDir)) * lights.pointLights[i].color.rgb;
        vertexOut.color.rgb += pointLightColor;
    }
    // Ensure color is clamped to [0, 1]
    vertexOut.color.rgb = clamp(vertexOut.color.rgb, 0.0f, 1.0f);

    return vertexOut;
}

fragment float4 fragmentTexturePHONG(
        VertexOut vertexOut [[stage_in]],
        texture2d<float> diffuseTexture [[texture(0)]],
        sampler textureSampler [[sampler(0)]]
) {
    // Sample the texture
    float4 texColor = diffuseTexture.sample(textureSampler, vertexOut.uv);

    // Combine the texture color with the vertex color
    // Manteniamo l'alpha originale della texture
    float4 finalColor;
    finalColor.rgb = vertexOut.color.rgb + texColor.rgb;
    finalColor.a = texColor.a; // Preserva il canale alpha della texture

    // Ensure final color is clamped to [0, 1]
    finalColor.rgb = clamp(finalColor.rgb, 0.0f, 1.0f);

    return finalColor;
}
