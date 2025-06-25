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

struct Uniforms {
    float4x4 modelViewProjectionMatrix;
};

vertex VertexOut vertexColorShader(
    Vertex vertexIn [[stage_in]],
    constant Uniforms& uniforms [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = uniforms.modelViewProjectionMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = vertexIn.color;
    return vertexOut;
}

fragment float4 fragmentColorShader(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}


