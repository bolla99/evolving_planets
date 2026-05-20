#include <metal_stdlib>
using namespace metal;

#include "Lighting.hpp"


struct Vertex {
    float3 position [[attribute(0)]] ;
};

struct DirectionalOut {
    float4 position [[position]];
    uint slice [[render_target_array_index]];
};

vertex DirectionalOut vertexDirectionalShadow(
    Vertex vertexIn [[stage_in]],
    constant float4x4& modelMatrix [[buffer(28)]],
    uint instanceID [[instance_id]],
    constant ShadowData& shadowData [[buffer(30)]]
) {
    DirectionalOut vertexOut;
    vertexOut.slice = instanceID;
    vertexOut.position = shadowData.viewProjectionMatrix[instanceID] * modelMatrix * float4(vertexIn.position, 1.0f);
    return vertexOut;
}

fragment float4 fragmentDirectionalShadow(DirectionalOut vertexOut [[stage_in]]) {
    return {1.0f, 1.0f, 1.0f, 1.0f};
}

struct PointOut {
    float4 position [[position]];
    float4 worldPosition;
    uint layerIndex [[render_target_array_index]];
    uint lightIndex;
};

vertex PointOut vertexPointShadow(
    Vertex vertexIn [[stage_in]],
    constant float4x4& modelMatrix [[buffer(28)]],
    uint instanceID [[instance_id]],
    constant PointShadowData& shadowData [[buffer(30)]]
) {
    PointOut out;
    out.worldPosition = modelMatrix * float4(vertexIn.position, 1.0f);
    out.layerIndex = instanceID;
    out.position = shadowData.matrices[out.layerIndex] * out.worldPosition;
    out.lightIndex = out.layerIndex / 6;
    return out;
}

struct FragmentOut {
    float depth [[depth(any)]];
};

fragment FragmentOut fragmentPointShadow(
                                   PointOut out [[stage_in]],
                                   constant PointShadowData& shadowData [[buffer(30)]]
                                   ) {
    FragmentOut fOut;
    fOut.depth = distance(out.worldPosition, shadowData.positions[out.lightIndex]) / shadowData.farPlanes[out.lightIndex].x;
    return fOut;
}
