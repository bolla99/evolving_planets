#include <metal_stdlib>
using namespace metal;

/**
 * Contrast Adaptive Sharpening (CAS) Compute Shader.
 *
 * Questo kernel applica un filtro di nitidezza adattivo al contrasto (ispirato ad AMD CAS).
 * Incrementa la nitidezza dei dettagli fini senza accentuare i bordi già molto contrastanti,
 * evitando così la comparsa di fastidiosi artefatti visivi ("halos" o bordi bianchi).
 *
 * Input:
 * - inputTexture: La texture da riscalare/processare (es. l'output di MetalFX)
 * - outputTexture: Texture di destinazione
 * - sharpness: Forza del filtro (compresa tra 0.0 e 1.0)
 */
kernel void sharpening_pass(
    texture2d<float, access::read>  inputTexture  [[texture(0)]],
    texture2d<float, access::write> outputTexture [[texture(1)]],
    constant float&                 sharpness     [[buffer(0)]],
    uint2                           gid           [[thread_position_in_grid]]
) {
    // Controllo dei confini della grid di esecuzione
    if (gid.x >= outputTexture.get_width() || gid.y >= outputTexture.get_height()) {
        return;
    }

    int2 pos = int2(gid);
    int2 size = int2(inputTexture.get_width(), inputTexture.get_height());

    // 5-tap neighborhood: Centro, Alto, Sinistra, Destra, Basso
    float3 c = inputTexture.read(uint2(pos)).rgb;
    float3 t = inputTexture.read(uint2(clamp(pos + int2(0, -1), int2(0), size - 1))).rgb;
    float3 l = inputTexture.read(uint2(clamp(pos + int2(-1, 0), int2(0), size - 1))).rgb;
    float3 r = inputTexture.read(uint2(clamp(pos + int2(1, 0), int2(0), size - 1))).rgb;
    float3 b = inputTexture.read(uint2(clamp(pos + int2(0, 1), int2(0), size - 1))).rgb;

    // Calcoliamo min e max RGB nel vicinato per misurare il contrasto locale
    float3 mn = min(min(min(min(c, t), l), r), b);
    float3 mx = max(max(max(max(c, t), l), r), b);

    // Calcolo del contrasto relativo
    float3 r_contrast = mx - mn;
    float3 c_inv = 1.0f / (mx + mn + 1e-5f);

    // Adattamento del contrasto: zone con contrasto moderato/basso vengono nitidizzate di più rispetto ai bordi netti
    float3 w = r_contrast * c_inv;
    float3 scale = 1.0f - w;

    // Portiamo lo sharpness parametrizzato [0.0 - 1.0] nel range di peso di CAS [-1/8 a -1/5]
    float active_sharpness = clamp(sharpness, 0.0f, 1.0f);
    float3 weight = -1.0f / mix(8.0f, 5.0f, active_sharpness * scale);

    // Risoluzione del filtro convolutivo
    float3 finalColor = (t + l + r + b) * weight + c;
    finalColor /= (1.0f + 4.0f * weight);

    // Clamping finale per prevenire overflow fuori dal range colore HDR o LDR standard
    finalColor = clamp(finalColor, 0.0f, 1.0f);

    // Scrittura finale conservando il canale alpha originale
    float alpha = inputTexture.read(uint2(pos)).a;
    outputTexture.write(float4(finalColor, alpha), gid);
}

