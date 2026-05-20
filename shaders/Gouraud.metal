#include <metal_stdlib>
#include <metal_geometric>
#include <metal_matrix>

using namespace metal;

#include "Lighting.hpp"

struct VertexOutGouraud {
    float4 position [[position]];
    float4 vertexColor;
    float4 shade;
    float2 uv;
};

vertex VertexOutGouraud vertexGOURAUD(
    VertexPCNUV vertexIn [[stage_in]],
    constant Lights& lights [[buffer(24)]],
    constant float4& cameraPosition [[buffer(25)]],
    constant float& shininess [[buffer(26)]],
    constant float3x3& normalMatrix [[buffer(27)]],
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
) {
    VertexOutGouraud vertexOut;
    // position
    vertexOut.position = projectionMatrix * viewMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.vertexColor = vertexIn.color;
    vertexOut.uv = vertexIn.uv;
    float4 worldPosition = (modelMatrix * float4(vertexIn.position, 1.0f));
    float4 normal = float4(float3x3(normalMatrix) * normalize(vertexIn.normal), 0.0f);
    vertexOut.shade = applyPHONGLights(worldPosition, float4(0.0f, 0.0f, 0.0f, 1.0f), normal, shininess, cameraPosition, lights);

    
    
    
    return vertexOut;
}

fragment float4 fragmentVCGOURAUD(
        VertexOutGouraud vertexOut [[stage_in]]
) {
    return vertexOut.vertexColor + vertexOut.shade;
}


fragment float4 fragmentTextureGOURAUD(
        VertexOutGouraud vertexOut [[stage_in]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        texture2d<float> diffuseTexture [[texture(0)]],
        sampler textureSampler [[sampler(0)]]
) {
    return diffuseTexture.sample(textureSampler, vertexOut.uv) + vertexOut.shade;
}

