#include <metal_stdlib>
using namespace metal;

#include <util.hpp>

#define MAX_POINT_LIGHTS 16
#define MAX_DIRECTIONAL_LIGHTS 4

struct VertexIn {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float4 currentClipPosition;
    float4 previousClipPosition;
};

vertex VertexOut vertexCurve(
    VertexIn vertexIn [[stage_in]],
    constant float4x4& modelMatrix [[buffer(26)]],
    constant float4x4& viewProjectionMatrix [[buffer(27)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(28)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(29)]]
) {
    VertexOut out;
    out.position = jitteredViewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    out.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    out.previousClipPosition = previousViewProjectionMatrix * modelMatrix * float4(vertexIn.position, 1.0f);
    out.color = vertexIn.color;

    return out;
}

struct FragmentOut {
    float4 color [[color(0)]];
    float2 motionVector [[color(1)]];
};

[[fragment]]
FragmentOut fragmentCurve(
        VertexOut in [[stage_in]]
) {
    return {in.color, motionVector(in.currentClipPosition, in.previousClipPosition)};
}
