#include <metal_stdlib>
using namespace metal;

// SMAA Blend Weight Calculation Pass - Versione migliorata
// Calcola i pesi di blending analizzando i pattern dei bordi rilevati

// Campiona gli edge vicini per determinare la continuità
float4 sample_edge(int x, int y, int width, int height, texture2d<float, access::read> edgeTexture) {
    int sx = clamp(x, 0, (int)width - 1);
    int sy = clamp(y, 0, (int)height - 1);
    return edgeTexture.read(uint2(sx, sy));
};

kernel void smaa_blend_weight(
    texture2d<float, access::read> edgeTexture [[texture(0)]],
    texture2d<float, access::write> blendTexture [[texture(1)]],
    constant float& intensity [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]])
{
    int2 coords = int2(gid);
    uint width = edgeTexture.get_width();
    uint height = edgeTexture.get_height();

    // Verifica limiti della texture
    if (coords.x >= (int)width || coords.y >= (int)height) {
        return;
    }

    // Leggi il valore dell'edge dal pass precedente
    float4 centerEdge = edgeTexture.read(uint2(coords));
    float centerEdgeStrength = centerEdge.z;

    // Inizializza il peso di blending
    float4 blendWeight = float4(0.0);

    if (centerEdgeStrength < 0.01) {
        // Nessun edge rilevato, nessun blending necessario
        blendTexture.write(blendWeight, uint2(coords));
        return;
    }

    float4 edgeLeft = sample_edge(coords.x - 1, coords.y, (int)width, (int)height, edgeTexture);
    float4 edgeRight = sample_edge(coords.x + 1, coords.y, (int)width, (int)height, edgeTexture);
    float4 edgeTop = sample_edge(coords.x, coords.y - 1, (int)width, (int)height, edgeTexture);
    float4 edgeBottom = sample_edge(coords.x, coords.y + 1, (int)width, (int)height, edgeTexture);

    float edgeLeftStrength = edgeLeft.z;
    float edgeRightStrength = edgeRight.z;
    float edgeTopStrength = edgeTop.z;
    float edgeBottomStrength = edgeBottom.z;

    // Calcola continuità dei bordi (edge connectivity)
    // Se l'edge continua nei pixel vicini, il blending dovrebbe essere più forte
    float leftContinuity = (edgeLeftStrength > 0.0) ? 1.0 : 0.0;
    float rightContinuity = (edgeRightStrength > 0.0) ? 1.0 : 0.0;
    float topContinuity = (edgeTopStrength > 0.0) ? 1.0 : 0.0;
    float bottomContinuity = (edgeBottomStrength > 0.0) ? 1.0 : 0.0;

    // Calcola i pesi di blending per i 4 spigoli
    // Questi pesi determinano quanto ammorbidire il bordo in ogni direzione

    // Peso per il left-top corner
    float leftTopWeight = centerEdgeStrength * topContinuity * intensity;

    // Peso per il right-top corner
    float rightTopWeight = centerEdgeStrength * topContinuity * intensity;

    // Peso per il left-bottom corner
    float leftBottomWeight = centerEdgeStrength * bottomContinuity * intensity;

    // Peso per il right-bottom corner
    float rightBottomWeight = centerEdgeStrength * bottomContinuity * intensity;

    // Ridistribuisci i pesi in base alla direzione del bordo
    // X = left, Y = top, Z = right, W = bottom
    if (centerEdge.w > 0.5) {
        // Bordo orizzontale - applica blending principalmente su top/bottom
        blendWeight.y = topContinuity > 0.0 ? centerEdgeStrength * intensity : 0.0;
        blendWeight.w = bottomContinuity > 0.0 ? centerEdgeStrength * intensity : 0.0;
    } else {
        // Bordo verticale - applica blending principalmente su left/right
        blendWeight.x = leftContinuity > 0.0 ? centerEdgeStrength * intensity : 0.0;
        blendWeight.z = rightContinuity > 0.0 ? centerEdgeStrength * intensity : 0.0;
    }

    // Clamp ai valori 0-1
    blendWeight = clamp(blendWeight, 0.0, 1.0);

    blendTexture.write(blendWeight, uint2(coords));
}

