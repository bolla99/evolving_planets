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

struct Lights {
    float4 globalAmbientLightColor;
    DirectionalLight directionalLights[MAX_DIRECTIONAL_LIGHTS];
    PointLight pointLights[MAX_POINT_LIGHTS];
};

// buffer 27
struct Material {
    uint8_t addTint;
    uint8_t _padding[15];
    float4 tintColor;
};

vertex VertexOut vertexVCPHONG(
    VertexPCN vertexIn [[stage_in]],
    constant Lights& lights [[buffer(28)]],
    constant float4x4& modelMatrix [[buffer(29)]],
    constant float4x4& viewProjectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = viewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    float3 worldPosition = (modelMatrix * float4(vertexIn.position, 1.0f)).xyz;
    vertexOut.color = vertexIn.color;
    
    // Calculate lighting
    float3 normal = normalize(vertexIn.normal);
    float3 worldNormal = (modelMatrix * float4(normal, 0.0f)).xyz;

    // apply global ambient light
    vertexOut.color.rgb += lights.globalAmbientLightColor.rgb;
    // apply directional light
    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i) {
        float3 directionalLightColor = max(0.0f, dot(worldNormal, -lights.directionalLights[i].direction)) * lights.directionalLights[i].color.rgb;
        vertexOut.color.rgb += directionalLightColor;
    }
    // apply point lights
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        float3 lightDir = normalize(lights.pointLights[i].position - worldPosition);
        float3 pointLightColor = max(0.0f, dot(worldNormal, lightDir)) * lights.pointLights[i].color.rgb;
        // set point light falloff
        float distance = length(lights.pointLights[i].position - worldPosition);
        pointLightColor *= 1.0f / (distance * distance);
        vertexOut.color.rgb += pointLightColor;
    }
    // Ensure color is clamped to [0, 1]
    vertexOut.color.rgb = clamp(vertexOut.color.rgb, 0.0f, 1.0f);

    return vertexOut;
}

vertex VertexOut vertexVCPHONGWithTint(
    VertexPCN vertexIn [[stage_in]],
    constant Material& material [[buffer(27)]],
    constant Lights& lights [[buffer(28)]],
    constant float4x4& modelMatrix [[buffer(29)]],
    constant float4x4& viewProjectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = viewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    float3 worldPosition = (modelMatrix * float4(vertexIn.position, 1.0f)).xyz;
    vertexOut.color = vertexIn.color;

    if (material.addTint > 0) { vertexOut.color *= material.tintColor; }

    // Calculate lighting
    float3 normal = normalize(vertexIn.normal);
    float3 worldNormal = (modelMatrix * float4(normal, 0.0f)).xyz;
    // apply global ambient light
    vertexOut.color.rgb += lights.globalAmbientLightColor.rgb;
    // apply directional light
    for (int i = 0; i < MAX_DIRECTIONAL_LIGHTS; ++i) {
        float3 directionalLightColor = max(0.0f, dot(worldNormal, -lights.directionalLights[i].direction)) * lights.directionalLights[i].color.rgb;
        vertexOut.color.rgb += directionalLightColor;
    }
    // apply point lights
    for (int i = 0; i < MAX_POINT_LIGHTS; ++i) {
        float3 lightDir = normalize(lights.pointLights[i].position - worldPosition);
        float3 pointLightColor = max(0.0f, dot(worldNormal, lightDir)) * lights.pointLights[i].color.rgb;
        // set point light falloff
        float distance = length(lights.pointLights[i].position - worldPosition);
        pointLightColor *= 1.0f / (distance * distance);
        vertexOut.color.rgb += pointLightColor;
    }
    // Ensure color is clamped to [0, 1]
    vertexOut.color.rgb = clamp(vertexOut.color.rgb, 0.0f, 1.0f);

    return vertexOut;
}

fragment float4 fragmentVCPHONG(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}


/*
fragment float4 fragmentVCPHONGWithTint(
    VertexOut vertexOut [[stage_in]],
    constant Material& material [[buffer(30)]]
) {
    if (material.addTint > 0) {
        vertexOut.color *= material.tintColor;
    }
    return vertexOut.color;
}
*/
