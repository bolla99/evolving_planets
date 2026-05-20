#include <metal_stdlib>
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]] ;
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertexUnlitShader(
    Vertex vertexIn [[stage_in]],
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
                                   
) {
    VertexOut vertexOut;
    vertexOut.position = projectionMatrix * viewMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = vertexIn.color;
    return vertexOut;
}

fragment float4 fragmentUnlitShader(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}


