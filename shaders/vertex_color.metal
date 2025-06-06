#include <metal_stdlib>
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
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
    constant float4* colors [[buffer(1)]],
    constant Uniforms& uniforms [[buffer(30)]]
) {
    float4 color = float4(1.0f, 0.0f, 1.0f, 1.0f);
    if (colors != nullptr) {
        color = vertexIn.color;
    }
    VertexOut vertexOut;
    vertexOut.position = uniforms.modelViewProjectionMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = color;
    return vertexOut;
}

fragment float4 fragmentColorShader(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}


