#include <metal_stdlib>
using namespace metal;

kernel void bloom_combine(
    texture2d<float, access::read>    inputHDR  [[texture(0)]],
    texture2d<float, access::sample>  bloomTex  [[texture(1)]],
    texture2d<float, access::write>   outputHDR [[texture(2)]],
    constant float&                   bloomIntensity [[buffer(0)]],
    uint2                             gid       [[thread_position_in_grid]]
) {
    if (gid.x >= outputHDR.get_width() || gid.y >= outputHDR.get_height()) return;

    float3 original = inputHDR.read(gid).rgb;

    // Output and Input HDR are same size. Bloom texture is smaller (1/2 size).
    float2 uv = (float2(gid) + 0.5f) / float2(outputHDR.get_width(), outputHDR.get_height());

    // Definiamo un bilinear sampler constexpr interno allo shader per interpolare la texture dimezzata
    constexpr sampler bilinearSampler(min_filter::linear, mag_filter::linear, mip_filter::none, address::clamp_to_edge);
    float3 bloomVal = bloomTex.sample(bilinearSampler, uv).rgb;

    float3 combined = original + bloomVal * bloomIntensity;

    outputHDR.write(float4(combined, 1.0f), gid);
}

