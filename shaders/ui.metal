#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float4 color [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 color;
};

// materials
struct Rect {
    float2 position;
    float2 size;
};
struct ViewportSize {
float2 size;
float2 padding;
};

vertex VertexOut vertexUI(
    VertexIn vertexIn [[stage_in]],
    constant Rect &rect [[buffer(2)]],
    constant ViewportSize &viewportSize [[buffer(3)]]
) {
    VertexOut vertexOut;
    vertexIn.position.xy = vertexIn.position.xy * rect.size + rect.position;
    vertexOut.position = float4(vertexIn.position, 1.0f);
    vertexOut.position.x = vertexOut.position.x / viewportSize.size.x * 2.0f - 1.0f;
    vertexOut.position.y = -vertexOut.position.y / viewportSize.size.y * 2.0f + 1.0f;
    vertexOut.position.z = 0.9f;
    vertexOut.color = vertexIn.color;
    return vertexOut;
}

fragment float4 fragmentUI(VertexOut vertexOut [[stage_in]]) {
    return vertexOut.color;
}
