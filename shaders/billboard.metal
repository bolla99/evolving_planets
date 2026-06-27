#include <metal_stdlib>

using namespace metal;

struct VertexIn {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};
struct VertexOut {
    float4 position [[position]];
    float2 uv;
    float4 clipSpacePos;
};

struct BillBoard {
    float4 position;
    float4 color;
    float size;
    int useDepth;
    float depth;
    float radius;
};

[[vertex]] VertexOut billboardVertexFunction(
                                             VertexIn in [[stage_in]],
                                             constant float4& cameraPosition [[buffer(10)]],
                                             constant float4x4& viewMatrix [[buffer(11)]],
                                             constant float4x4& projectionMatrix [[buffer(12)]],
                                             constant BillBoard& billBoard [[buffer(13)]]
                                   ) {
    auto pos = billBoard.position;
    auto dir = normalize(cameraPosition - pos);
    pos += dir * billBoard.radius;
    auto viewSpacePos = viewMatrix * pos;
    viewSpacePos.xy += billBoard.size * in.position.xy;
    auto out = VertexOut();
    out.position = projectionMatrix * viewSpacePos;
    if (billBoard.useDepth > 0) {
        out.position.z = billBoard.depth * out.position.w;
    }

    // Riduciamo l'offset di profondità per evitare occlusioni errate o clipping
    // Invece di un valore fisso in clip space, usiamo un offset proporzionale a w
    //out.position.z -= billBoard.radius * out.position.w;

    out.clipSpacePos = out.position;
    out.uv = in.uv;
    return out;
}

// Funzione helper per linearizzare la profondità (trasforma i valori 0-1 in metri reali)
float linearizeDepth(float sampleDepth, float nearPlane, float farPlane) {
    return (nearPlane * farPlane) / (farPlane - sampleDepth * (farPlane - nearPlane));
}

[[fragment]] float4 billboardFragmentFunction(
                                                VertexOut in [[stage_in]],
                                                texture2d<float> billBoardTexture [[texture(0)]],
                                                texture2d<float, access::read> depthTexture [[texture(1)]],
                                                sampler billBoardSampler [[sampler(0)]],
                                                constant BillBoard& billBoard [[buffer(0)]],
                                                constant float2& cameraPlanes [[buffer(1)]],
                                                constant float& particleSoftness [[buffer(2)]]
) {
    float2 screenUV = in.clipSpacePos.xy / in.clipSpacePos.w;
    screenUV = screenUV * 0.5f + 0.5f;
    screenUV.y = 1.0f - screenUV.y;

    auto pixelX = screenUV.x * depthTexture.get_width();
    auto pixelY = screenUV.y * depthTexture.get_height();

    auto depth = depthTexture.read(uint2(pixelX, pixelY)).r;
    auto linearDepth = linearizeDepth(depth, cameraPlanes.x, cameraPlanes.y);
    auto linearParticleDepth = linearizeDepth(in.clipSpacePos.z / in.clipSpacePos.w, cameraPlanes.x, cameraPlanes.y);

    auto difference = linearDepth - linearParticleDepth;
    
    auto c =  billBoard.color * billBoardTexture.sample(billBoardSampler, in.uv);

    // differen >= 0.0f -> particle is in front of something
    if (difference >= 0.0f) {
        auto fade = saturate(difference / particleSoftness);
        c *= fade;
    } else {
        discard_fragment();
    }

    return c;
}



