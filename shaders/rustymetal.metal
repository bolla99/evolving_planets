#include <metal_stdlib>
#include <metal_geometric>
#include <metal_matrix>

using namespace metal;

#include "Lighting.hpp"
#include "util.hpp"

struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float4 normal;
    float4 color;
    float2 uv;
    float3 localPosition;
    float3 instanceWorldPosition [[flat]];
    float4 currentClipPosition;
    float4 previousClipPosition;
};

vertex VertexOut rustyVertex(
    VertexPCNUV in [[stage_in]],
    constant float4x4& previousModelMatrix [[buffer(25)]],
    constant float3x3& normalMatrix [[buffer(26)]],
    constant float4x4& modelMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(29)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(30)]]
) {
    VertexOut out;
    // position
    out.worldPosition = (modelMatrix * float4(in.position, 1.0f));
    out.position = jitteredViewProjectionMatrix * out.worldPosition;
    out.color = in.color;
    out.normal = normalize(float4(float3x3(normalMatrix) * in.normal, 0.0f));
    out.uv = in.uv;
    out.instanceWorldPosition = modelMatrix.columns[3].xyz;
    out.localPosition = in.position;
    out.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(in.position, 1.0);
    out.previousClipPosition = previousViewProjectionMatrix * previousModelMatrix * float4(in.position, 1.0);

    return out;
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut rustyFragment(
        VertexOut in [[stage_in]],
        constant float& alpha [[buffer(22)]],
        constant ShadowData& shadowData [[buffer(23)]],
        constant PointShadowData& pointShadowData [[buffer(24)]],
        constant float& roughness [[buffer(25)]],
        constant float& metallic [[buffer(26)]],
        constant float4& cameraPosition [[buffer(27)]],
        constant Lights& lights [[buffer(28)]],
        depth2d_array<float> shadowMaps [[texture(0)]],
        depthcube_array<float> cubeMaps [[texture(1)]],
        sampler textureSampler [[sampler(0)]]
) {

    float3 finalAlbedo = in.color.xyz;
    float finalRoughness = roughness;
    float finalMetallic = metallic;

    bool rusty = true;
    if (rusty) {
        // 1. Calcola il rumore procedurale usando la posizione LOCALE dei vertici
            // Moltiplica 'in.localPosition' per scalare la dimensione delle macchie di ruggine
            float rustScale = 1.0f;
            float rustAmount = fbm_from_perlin(in.localPosition * rustScale, 10);

            // 2. Crea la maschera di usura/ruggine
            // I parametri controllano la quantità di ruggine (es. se la soglia è bassa, ci sarà più ruggine)
            float sogliaRuggine = 0.45f;
            float sfumaturaRuggine = 0.2f;
            float rustMask = smoothstep(sogliaRuggine, sogliaRuggine + sfumaturaRuggine, rustAmount);

            // 3. DEFINIZIONE DEI MATERIALI (PBR)
            // Stato Cromo/Metallo pulito
            float3 metalColor = float3(0.8f, 0.82f, 0.83f); // Colore dell'acciaio
            float metalRoughness = 0.15f;                   // Super liscio e riflettente
            float metalMetallic = 0.95f;                     // Totalmente metallico

            // Stato Ruggine/Corrosione
            float3 rustColor = float3(0.35f, 0.17f, 0.08f);  // Marrone ruggine opaco
            float rustRoughness = 0.85f;                    // Molto rugoso, disperde la luce
            float rustMetallic = 0.0f;                      // La ruggine NON è un metallo

            // 4. MISCELAZIONE IN BASE ALLA MASCHERA PROCEDURALE
            finalAlbedo    = mix(metalColor, rustColor, rustMask);
            finalRoughness = mix(metalRoughness, rustRoughness, rustMask);
            finalMetallic  = mix(metalMetallic, rustMetallic, rustMask);
    }

    auto c = applyFULLSHADOWWARDLightsWithOrenNayar(
                                {float4(finalAlbedo, 1.0), in.normal, in.worldPosition},
                                 finalRoughness, finalMetallic,
                                 cameraPosition, lights, float3(1.0),
                                 shadowData,
                                 pointShadowData,
                                 shadowMaps,
                                 cubeMaps,
                                 textureSampler, nullptr, nullptr, 0, 0.0f, 0.001f, false
                                 );

    // DITHERING
    float dist = distance(in.instanceWorldPosition, cameraPosition.xyz);
    auto fadeNearDistance = 3.0f;
    auto fadeFarDistance = 10.0f;
    float a = smoothstep(fadeNearDistance, fadeFarDistance, dist);

    float threshold = interleavedGradientNoise(in.position.xy);
    if (a < threshold) {
        discard_fragment();
    }

    FragmentOut out;
    out.color = float4(c.rgb, c.a * alpha);
    out.motionVector = motionVector(in.currentClipPosition, in.previousClipPosition);
    return out;
}
