#include <metal_stdlib>
using namespace metal;

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct VertexIn {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

vertex VertexOut vertexCurve(
    VertexIn vertexIn [[stage_in]],
    constant float4x4& modelMatrix [[buffer(29)]],
    constant float4x4& viewProjectionMatrix [[buffer(30)]]
) {
    VertexOut vertexOut;
    vertexOut.position = viewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    vertexOut.color = vertexIn.color;

    return vertexOut;
}

fragment float4 fragmentCurve(
        VertexOut vertexOut [[stage_in]]
) {
    return vertexOut.color;
}
