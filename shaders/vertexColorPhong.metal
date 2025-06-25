#include <metal_stdlib>
using namespace metal;

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct VertexPCN {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
    float3 normal [[attribute(2)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

struct DirectionalLight {
    float3 direction;
    float4 color;
};

struct PointLight {
    float3 position;
    float4 color;
};

struct Uniforms {
    float4x4 modelViewProjectionMatrix;
};

struct Lights {
    float4 globalAmbientLightColor;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
    int numDirectionalLights;
    int numPointLights;
    int _padding[2];
};



vertex VertexOut vertexVCPHONG(
    VertexPCN vertexIn [[stage_in]],
    constant Lights& lights [[buffer(29)]],
    constant Uniforms& uniforms [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = uniforms.modelViewProjectionMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = vertexIn.color;
    
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

fragment float4 fragmentVCPHONG(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}
