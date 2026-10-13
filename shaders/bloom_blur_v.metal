#include <metal_stdlib>
using namespace metal;

kernel void bloom_blur_v(
    texture2d<float, access::read>  inputTex  [[texture(0)]],
    texture2d<float, access::write> outputTex [[texture(1)]],
    uint2                           gid       [[thread_position_in_grid]]
) {
    if (gid.x >= outputTex.get_width() || gid.y >= outputTex.get_height()) return;

    float3 color = float3(0.0f);
    float weights[5] = { 0.227027f, 0.1945946f, 0.1216216f, 0.054054f, 0.016216f };

    color += inputTex.read(gid).rgb * weights[0];
    for (int i = 1; i < 5; ++i) {
        uint2 p_pos = uint2(gid.x, clamp(int(gid.y) + i, 0, int(inputTex.get_height() - 1)));
        uint2 n_pos = uint2(gid.x, clamp(int(gid.y) - i, 0, int(inputTex.get_height() - 1)));
        color += inputTex.read(p_pos).rgb * weights[i];
        color += inputTex.read(n_pos).rgb * weights[i];
    }

    outputTex.write(float4(color, 1.0f), gid);
}

