#include <metal_stdlib>
using namespace metal;

vertex float4 vertexShader(uint vertexID [[vertex_id]],
                           constant simd::float3* vertices) {
    auto vertexOut = float4(
                            vertices[vertexID][0],
                            vertices[vertexID][1],
                            vertices[vertexID][2],
                            1.0f
                            );
    return vertexOut;
}

fragment float4 fragmentShader(float4 vertexOut [[stage_in]]) {
    return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

