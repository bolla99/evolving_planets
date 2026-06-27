#include <metal_stdlib>
#include "util.hpp"
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]] ;
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
    float4 currentClipPosition;
    float4 previousClipPosition;
};


vertex VertexOut vertexUnlitShader(
    Vertex in [[stage_in]],
    constant float4x4& previousModelMatrix [[buffer(25)]],
    constant float4& unlitColor [[buffer(26)]],
    constant float4x4& modelMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(29)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(30)]]
                                   
) {
    VertexOut out;
    out.position = jitteredViewProjectionMatrix * modelMatrix * float4(in.position, 1.0f);
    out.color = unlitColor;
    out.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(in.position, 1.0);
    out.previousClipPosition = previousViewProjectionMatrix * previousModelMatrix * float4(in.position, 1.0);
    return out;
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut fragmentUnlitShader(VertexOut vertexOut [[stage_in]]) {
    FragmentOut out;
    out.color = vertexOut.color;
    out.motionVector = motionVector(vertexOut.currentClipPosition, vertexOut.previousClipPosition);
    return out;
}


