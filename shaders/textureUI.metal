#include <metal_stdlib>
using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 texCoord [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float2 texCoord;
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


vertex VertexOut vertexTextureUI(
    VertexIn vertexIn [[stage_in]],
    constant Rect &rect [[buffer(2)]],
    constant ViewportSize &viewportSize [[buffer(3)]]
) {
    VertexOut vertexOut;
    vertexIn.position.xy = vertexIn.position.xy * rect.size + rect.position;
    vertexOut.position = float4(vertexIn.position, 1.0f);
    vertexOut.position.x = vertexOut.position.x / viewportSize.size.x * 2.0f - 1.0f;
    vertexOut.position.y = -vertexOut.position.y / viewportSize.size.y * 2.0f + 1.0f;
    vertexOut.texCoord = vertexIn.texCoord;
    return vertexOut;
}

fragment float4 fragmentTextureUI(
    VertexOut vertexOut [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    sampler textureSampler [[sampler(0)]]
) {
    float4 texColor = texture.sample(textureSampler, vertexOut.texCoord);
    /*
    if (texColor.a < 0.01) {
        //discard_fragment();
    }*/
    return texColor;
}


fragment float4 fragmentDepth(
    VertexOut vertexOut [[stage_in]],
    depth2d_array<float> depthTexture [[texture(0)]],
    sampler textureSampler [[sampler(0)]]
) {
    float depth = depthTexture.sample(textureSampler, vertexOut.texCoord, 0);
    //depth = depth * 1.0f / (10.0f + depth * (1.0f - 10.0f));
    return {depth, depth, depth, 1.0f};
}
