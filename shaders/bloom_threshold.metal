#include <metal_stdlib>
using namespace metal;

constant float3 LUMINANCE_VECTOR = float3(0.2126, 0.7152, 0.0722);

kernel void bloom_threshold(
    texture2d<float, access::read>  inputHDR  [[texture(0)]],
    texture2d<float, access::write> outputHDR [[texture(1)]],
    constant float&                 threshold [[buffer(0)]],
    uint2                           gid       [[thread_position_in_grid]]
) {
    if (gid.x >= outputHDR.get_width() || gid.y >= outputHDR.get_height()) return;

    // Map output coordinates to input texture (output is 1/2 size)
    float2 uv = (float2(gid) + 0.5f) / float2(outputHDR.get_width(), outputHDR.get_height());
    uint2 inputCoord = uint2(uv * float2(inputHDR.get_width(), inputHDR.get_height()));

    float3 c0 = inputHDR.read(inputCoord + uint2(0, 0)).rgb;
    float3 c1 = inputHDR.read(clamp(inputCoord + uint2(1, 0), uint2(0), uint2(inputHDR.get_width() - 1, inputHDR.get_height() - 1))).rgb;
    float3 c2 = inputHDR.read(clamp(inputCoord + uint2(0, 1), uint2(0), uint2(inputHDR.get_width() - 1, inputHDR.get_height() - 1))).rgb;
    float3 c3 = inputHDR.read(clamp(inputCoord + uint2(1, 1), uint2(0), uint2(inputHDR.get_width() - 1, inputHDR.get_height() - 1))).rgb;
    float3 avgColor = (c0 + c1 + c2 + c3) * 0.25f;

    float brightness = dot(avgColor, LUMINANCE_VECTOR);
    float3 bloomColor = float3(0.0f);

    if (brightness > threshold) {
        bloomColor = avgColor * (brightness - threshold) / max(brightness, 0.0001f);
    }

    outputHDR.write(float4(bloomColor, 1.0f), gid);
}

