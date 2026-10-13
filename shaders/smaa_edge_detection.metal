#include <metal_stdlib>
using namespace metal;

// SMAA Edge Detection Pass - Versione migliorata con Sobel operator
// Detecta discontinuità nei colori per identificare i bordi con maggiore accuratezza


// Campiona i 9 pixel attorno al centro (3x3 kernel)
float4 sample(int x, int y, float width, float height, texture2d<float, access::read> inputTexture) {
    int sx = clamp(x, 0, (int)width - 1);
    int sy = clamp(y, 0, (int)height - 1);
    return inputTexture.read(uint2(sx, sy));
};

// Converti a luminanza (Rec. 709)
float toLuma(float4 c) {
    return dot(c.rgb, float3(0.2126, 0.7152, 0.0722));
};

kernel void smaa_edge_detection(
    texture2d<float, access::read> inputTexture [[texture(0)]],
    texture2d<float, access::write> edgeTexture [[texture(1)]],
    constant float& threshold [[buffer(0)]],
    uint2 gid [[thread_position_in_grid]])
{
    int2 coords = int2(gid);
    uint width = inputTexture.get_width();
    uint height = inputTexture.get_height();

    // Verifica limiti della texture
    if (coords.x >= (int)width || coords.y >= (int)height) {
        return;
    }

    // Leggi i colori della finestra 3x3
    float4 tl = sample(coords.x - 1, coords.y - 1, width, height, inputTexture); // top-left
    float4 tm = sample(coords.x,     coords.y - 1, width, height, inputTexture); // top-middle
    float4 tr = sample(coords.x + 1, coords.y - 1, width, height, inputTexture); // top-right

    float4 ml = sample(coords.x - 1, coords.y, width, height, inputTexture);     // mid-left
    float4 mm = sample(coords.x,     coords.y, width, height, inputTexture);    // mid-middle (center)
    float4 mr = sample(coords.x + 1, coords.y, width, height, inputTexture);     // mid-right

    float4 bl = sample(coords.x - 1, coords.y + 1, width, height, inputTexture); // bottom-left
    float4 bm = sample(coords.x,     coords.y + 1, width, height, inputTexture); // bottom-middle
    float4 br = sample(coords.x + 1, coords.y + 1, width, height, inputTexture); // bottom-right

    float l_tl = toLuma(tl);
    float l_tm = toLuma(tm);
    float l_tr = toLuma(tr);
    float l_ml = toLuma(ml);
    float l_mm = toLuma(mm);
    float l_mr = toLuma(mr);
    float l_bl = toLuma(bl);
    float l_bm = toLuma(bm);
    float l_br = toLuma(br);

    // Applica l'operatore Sobel per calcolare i gradienti
    // Gradient X (edge verticali)
    float sobelX = (-l_tl - 2.0 * l_ml - l_bl) + (l_tr + 2.0 * l_mr + l_br);

    // Gradient Y (edge orizzontali)
    float sobelY = (-l_tl - 2.0 * l_tm - l_tr) + (l_bl + 2.0 * l_bm + l_br);

    // Magnitudine del gradiente (forza totale del bordo)
    float edgeMagnitude = sqrt(sobelX * sobelX + sobelY * sobelY);

    // Normalizza i gradienti per ottenere la direzione del bordo
    float2 edgeDirection = normalize(float2(sobelX, sobelY) + 0.0001); // +0.0001 per evitare divisione per zero

    // Determina se il bordo è più orizzontale o verticale
    bool isHorizontalEdge = abs(sobelX) > abs(sobelY);

    // Applica la soglia (parametro modificabile) - versione più aggressiva
    float adaptiveThreshold = max(threshold * 0.3, 0.01);
    float edgeStrength = smoothstep(adaptiveThreshold, threshold * 3.0, edgeMagnitude);

    // Output:
    // R: componente X della direzione del bordo normalizzata
    // G: componente Y della direzione del bordo normalizzata
    // B: forza del bordo (0-1)
    // A: indicatore se orizzontale (1.0) o verticale (0.0)
    edgeTexture.write(float4(
        edgeDirection.x * 0.5 + 0.5,  // normalizza a 0-1 range
        edgeDirection.y * 0.5 + 0.5,  // normalizza a 0-1 range
        edgeStrength,
        isHorizontalEdge ? 1.0 : 0.0
    ), uint2(coords));
}

