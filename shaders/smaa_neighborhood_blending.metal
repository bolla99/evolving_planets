#include <metal_stdlib>
using namespace metal;

// SMAA Neighborhood Blending Pass - Versione migliorata
// Applica il blending finale usando i pesi calcolati e un kernel intelligente

// Campiona i colori vicini con un kernel 3x3
float4 sampleColor(int x, int y, float width, float height, texture2d<float, access::read> inputTexture) {
    int sx = clamp(x, 0, (int)width - 1);
    int sy = clamp(y, 0, (int)height - 1);
    return inputTexture.read(uint2(sx, sy));
};

kernel void smaa_neighborhood_blending(
    texture2d<float, access::read> inputTexture [[texture(0)]],
    texture2d<float, access::read> blendTexture [[texture(1)]],
    texture2d<float, access::write> outputTexture [[texture(2)]],
    constant float& blendingIntensity [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]])
{
    int2 coords = int2(gid);
    uint width = inputTexture.get_width();
    uint height = inputTexture.get_height();

    // Verifica limiti della texture
    if (coords.x >= (int)width || coords.y >= (int)height) {
        return;
    }

    // Leggi il colore centrale
    float4 centerColor = inputTexture.read(uint2(coords));

    // Leggi il peso di blending per questo pixel
    float4 blendWeight = blendTexture.read(uint2(coords));
    float totalBlend = dot(blendWeight, float4(1.0));

    // Se nessun blending è necessario, restituisci il colore originale
    if (totalBlend < 0.01) {
        outputTexture.write(centerColor, uint2(coords));
        return;
    }

    float4 colorLeft = sampleColor(coords.x - 1, coords.y, width, height, inputTexture);
    float4 colorRight = sampleColor(coords.x + 1, coords.y, width, height, inputTexture);
    float4 colorTop = sampleColor(coords.x, coords.y - 1, width, height, inputTexture);
    float4 colorBottom = sampleColor(coords.x, coords.y + 1, width, height, inputTexture);

    // Campiona anche i colori diagonali per un blending più morbido
    float4 colorTopLeft = sampleColor(coords.x - 1, coords.y - 1, width, height, inputTexture);
    float4 colorTopRight = sampleColor(coords.x + 1, coords.y - 1, width, height, inputTexture);
    float4 colorBottomLeft = sampleColor(coords.x - 1, coords.y + 1, width, height, inputTexture);
    float4 colorBottomRight = sampleColor(coords.x + 1, coords.y + 1, width, height, inputTexture);

    // Inizializza il risultato con il colore centrale
    float4 result = centerColor * (1.0 - totalBlend);

    // Applica il blending in base ai pesi calcolati
    // X = left, Y = top, Z = right, W = bottom

    // Blending orizzontale (left/right)
    if (blendWeight.x > 0.01) {
        // Blend con il pixel a sinistra
        result += colorLeft * blendWeight.x * blendingIntensity;
    }
    if (blendWeight.z > 0.01) {
        // Blend con il pixel a destra
        result += colorRight * blendWeight.z * blendingIntensity;
    }

    // Blending verticale (top/bottom)
    if (blendWeight.y > 0.01) {
        // Blend con il pixel in alto
        result += colorTop * blendWeight.y * blendingIntensity;
    }
    if (blendWeight.w > 0.01) {
        // Blend con il pixel in basso
        result += colorBottom * blendWeight.w * blendingIntensity;
    }

    // Normalizza il risultato se il blending è stato applicato
    float weightSum = 1.0 - totalBlend + (dot(blendWeight, float4(1.0)) * blendingIntensity);
    if (weightSum > 0.0) {
        result /= weightSum;
    }

    outputTexture.write(result, uint2(coords));
}

