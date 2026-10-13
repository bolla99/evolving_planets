#include <metal_stdlib>
using namespace metal;

static inline float3 ACESFilm(float3 x) {
    float a = 2.51f;
    float b = 0.03f;
    float c = 2.43f;
    float d = 0.59f;
    float e = 0.14f;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0f, 1.0f);
}

kernel void tonemapping_pass(
    texture2d<float, access::read>  inputHDR  [[texture(0)]],
    texture2d<float, access::write> outputLDR [[texture(1)]],
    uint2                           gid       [[thread_position_in_grid]]
) {
    if (gid.x >= outputLDR.get_width() || gid.y >= outputLDR.get_height()) return;

    float3 hdrColor = inputHDR.read(gid).rgb;
    float3 ldrColor = ACESFilm(hdrColor);

    float alpha = inputHDR.read(gid).a;
    outputLDR.write(float4(ldrColor, alpha), gid);
}

