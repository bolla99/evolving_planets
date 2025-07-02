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

vertex VertexOut vertexTextureUI(
    VertexIn vertexIn [[stage_in]],
    constant float2 &viewportSize [[buffer(20)]]
) {
    VertexOut vertexOut;
    vertexOut.position = float4(vertexIn.position, 1.0f);
    vertexOut.position.x = vertexOut.position.x / viewportSize.x * 2.0f - 1.0f;
    vertexOut.position.y = -vertexOut.position.y / viewportSize.y * 2.0f + 1.0f;
    vertexOut.texCoord = vertexIn.texCoord;
    return vertexOut;
}

fragment float4 fragmentTextureUI(
    VertexOut vertexOut [[stage_in]],
    texture2d<float> texture [[texture(0)]],
    sampler textureSampler [[sampler(0)]]
) {
    float4 texColor = texture.sample(textureSampler, vertexOut.texCoord);
    if (texColor.a < 0.01) {
        //discard_fragment();
    }
    return texColor;
}
