#include <metal_stdlib>
#include <metal_geometric>
#include <metal_matrix>

using namespace metal;

#include "Lighting.hpp"

struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float4 normal;
    float4 color;
    float2 uv;
};

vertex VertexOut vertexPHONG(
    VertexPCNUV vertexIn [[stage_in]],
    constant float3x3& normalMatrix [[buffer(27)]],
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    // position
    vertexOut.position = projectionMatrix * viewMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.worldPosition = (modelMatrix * float4(vertexIn.position, 1.0f));
    vertexOut.color = vertexIn.color;
    vertexOut.normal = float4(float3x3(normalMatrix) * normalize(vertexIn.normal), 0.0f);
    vertexOut.uv = vertexIn.uv;
    
    return vertexOut;
}

fragment float4 fragmentVCPHONG(
        VertexOut vertexOut [[stage_in]],
        constant float& shininess [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]]
) {
    float4 position = vertexOut.worldPosition;
    float4 color = vertexOut.color;
    float4 normal = normalize(vertexOut.normal);
    
    return applyPHONGLights(position, color, normal, shininess, cameraPosition, lights);
}


fragment float4 fragmentTexturePHONG(
        VertexOut vertexOut [[stage_in]],
        constant float& shininess [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        texture2d<float> diffuseTexture [[texture(0)]],
        sampler textureSampler [[sampler(0)]]
) {
    float4 position = vertexOut.worldPosition;
    float4 texColor = diffuseTexture.sample(textureSampler, vertexOut.uv);
    float4 normal = normalize(vertexOut.normal);
    
    return applyPHONGLights(position, texColor, normal, shininess, cameraPosition, lights);
}



