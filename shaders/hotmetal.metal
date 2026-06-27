#include <metal_stdlib>
#include "util.hpp"
using namespace metal;

struct Vertex {
    float3 position [[attribute(0)]];
    float2 uv [[attribute(1)]];
};

struct VertexOut {
    float4 position [[position]];
    float4 localPosition;
    float2 uv;
    float4 currentClipPosition;
    float4 previousClipPosition;
};

vertex VertexOut hotMetalVertex(
    Vertex in [[stage_in]],
    constant float4x4& previousModelMatrix [[buffer(25)]],
    constant float4x4& modelMatrix [[buffer(27)]],
    constant float4x4& viewProjectionMatrix [[buffer(28)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(29)]],
    constant float4x4& jitteredViewProjectionMatrix [[buffer(30)]]
) {
    VertexOut out;
    out.position = jitteredViewProjectionMatrix * modelMatrix * float4(in.position, 1.0f);
    out.localPosition = float4(in.position, 1);
    out.uv = in.uv;
    out.currentClipPosition = viewProjectionMatrix * modelMatrix * float4(in.position, 1.0);
    out.previousClipPosition = previousViewProjectionMatrix * previousModelMatrix * float4(in.position, 1.0);
    return out;
}

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut hotMetalFragment(
    VertexOut in [[stage_in]],
    constant float& heat [[buffer(10)]],
    constant float& time [[buffer(11)]]
) {
    float2 uvCentrate = in.uv - float2(0.5f, 0.5f);
        float distanzaRadiale = length(uvCentrate) * 2.0f;
        
        // Usiamo lo smoothstep direttamente sulla geometria di base per ammorbidire il profilo
        float caloreRadiale = smoothstep(1.0f, 0.0f, saturate(distanzaRadiale));
        float calorePunto = saturate(caloreRadiale * heat);

        // Tavolozza riequilibrata (aggiunto un rosso-arancio intermedio tra rosso e blu)
        float3 c0 = float3(0.08f, 0.09f, 0.1f);  // Metallo spento
        float3 c1 = float3(0.5f, 0.02f, 0.0f);   // Rosso cupo
        float3 c2 = float3(0.9f, 0.3f, 0.0f);   // Arancione vivo
        float3 c3 = float3(0.1f, 0.5f, 1.0f);   // Blu plasma
        float3 c4 = float3(1.0f, 1.0f, 1.0f);   // Bianco incandescente

        // INTERPOLAZIONE CASCATA ULTRA-FLUIDA (Senza gradini matematici)
        // Usiamo funzioni polinomiali lisce su tutto il range unificato
        float t1 = smoothstep(0.00f, 0.30f, calorePunto);
        float t2 = smoothstep(0.25f, 0.60f, calorePunto);
        float t3 = smoothstep(0.50f, 0.85f, calorePunto);
        float t4 = smoothstep(0.75f, 1.00f, calorePunto);

        float3 coloreFinale = mix(c0, c1, t1);
        coloreFinale         = mix(coloreFinale, c2, t2);
        coloreFinale         = mix(coloreFinale, c3, t3);
        coloreFinale         = mix(coloreFinale, c4, t4);

        // Glow e Flicker coerenti
        float intensitaGlow = 1.0f + (heat * heat * 3.0f);
        coloreFinale *= intensitaGlow;

        float flicker = sin(time * 45.0f) * (0.02f * heat) + 1.0f;
        coloreFinale *= flicker;

        FragmentOut out;
        out.color = float4(coloreFinale, 1.0f);
        out.motionVector = motionVector(in.currentClipPosition, in.previousClipPosition);
        return out;
}


