#include <metal_stdlib>
#include "util.hpp"

using namespace metal;


struct SkyboxSettings {
    float density = 50.0;
    float starsFraction = 0.9;
};


struct VertexIn {
    float3 position [[attribute(0)]];
};

struct VertexOut {
    float4 position [[position]];
};

// materials
struct Rect {
    float2 position;
    float2 size;
};
struct ViewportSize {
float2 size;
float2 padding;
};

float3 getStarColor(float t) {
    // Spettro: Rosso -> Arancio -> Bianco -> Azzurro -> Blu
    if (t < 0.2) return float3(1.0, 0.4, 0.3); // Rosso/M
    if (t < 0.4) return float3(1.0, 0.7, 0.4); // Arancio/K
    if (t < 0.7) return float3(1.0, 1.0, 1.0); // Bianco/A-F (le più comuni)
    if (t < 0.9) return float3(0.7, 0.8, 1.0); // Azzurro/B
    return float3(0.5, 0.6, 1.0);              // Blu/O
}

float3 getStarColorSaturated(float t) {
    // Tavolozza potenziata con colori MOLTO saturi e meno bianchi
    // Intervalli: Blu Profondo -> Azzurro -> Giallo -> Arancio -> Rosso Intenso
    if (t < 0.15) return float3(0.1, 0.2, 1.0); // Blu profondo (molto saturo)
    if (t < 0.35) return float3(0.5, 0.7, 1.0); // Azzurro
    if (t < 0.65) return float3(1.0, 1.0, 1.0); // Bianco (G-F), base
    if (t < 0.85) return float3(1.0, 0.9, 0.5); // Giallo tenue
    return float3(1.0, 0.3, 0.1);              // Rosso intenso/Arancio (M)
}

vertex VertexOut vertexSkybox(
    VertexIn vertexIn [[stage_in]],
    constant Rect &rect [[buffer(2)]],
    constant ViewportSize &viewportSize [[buffer(3)]]
) {
    VertexOut vertexOut;
    vertexIn.position.xy = vertexIn.position.xy * rect.size + rect.position;
    vertexOut.position = float4(vertexIn.position, 1.0f);
    vertexOut.position.x = vertexOut.position.x / viewportSize.size.x * 2.0f - 1.0f;
    vertexOut.position.y = -vertexOut.position.y / viewportSize.size.y * 2.0f + 1.0f;
    return vertexOut;
}

inline float3 getPixelColor(float3 id, float3 dir, float starsFraction) {
    // h is the probability that in that cube a star is present
    float h = hash1d(id * 12.9898 + id.yzx * 78.233);
    
    // if h > starsFraction then the star is present
    if (h < starsFraction) return float3(0.0, 0.0, 0.0);
    
    // star position inside the cube
    float3 offset = hash3d_point(id);
        
    // absolute star position
    float3 starPos = id + offset;// / 2.0;
    float3 starDir = normalize(starPos);
        
    // distance between pixel and star
    float cosAngle = dot(dir, starDir);
        
    if (cosAngle <= 0.999) return float3(0.0, 0.0, 0.0);
    
    // use 128 (good value)
    // the probability h becomes the size selector; the higher, the bigger
    float sizeSelector = pow(h, 128);
            
    // exponents for core and glow; the lower, the higher
    float coreExp = mix(10000000.0, 5000000.0, sizeSelector);
    float glowExp = mix(1000000.0,  500000.0,  sizeSelector);
            
    // brightness with size selector
    float brightness = mix(0.1, 1.0, sizeSelector);
            
    float core = pow(max(0.0, cosAngle), coreExp) * 2.0;
    float glow = pow(max(0.0, cosAngle), glowExp) * 0.8;
            
    // coordinates system where starDir is the front vector
    float3 up = abs(starDir.y) < 0.9 ? float3(0, 1, 0) : float3(1, 0, 0);
    float3 right = normalize(cross(starDir, up));
    float3 top = cross(starDir, right);
        
    float angle = 0.785; // 45° in radians
    // axes rotated by 45° on the xy plane (+x-y and +x+y)
    float3 rotRight = right * cos(angle) - top * sin(angle);
    float3 rotTop   = right * sin(angle) + top * cos(angle);
            
    // distance between star and pixel along spike directions
    float xDist = dot(dir, rotRight);
    float yDist = dot(dir, rotTop);
            
    // star has spikes if h >= 0.85 (
    //float hasSpikes = step(0.85, h); // H è il valore di hash della stella
    //presence = 1.0;
    float totalSpikes = 0.0f;
    
    // if h >= 0.85 spikes are rendered
    if (h >= 0.85) {
        /*
        float spikeLengthNE = mix(400.0, 100.0, sizeSelector);
        float spikeLengthNW = mix(400.0, 100.0, sizeSelector); //offset.x); // Altra variazione casuale
        float spikeLengthSE = mix(400.0, 100.0, sizeSelector);
        float spikeLengthSW = mix(400.0, 100.0, sizeSelector);
        float spikeNE = pow(max(0.0, 1.0 - abs(xDist) * spikeLengthNE), 4.0);
        float spikeNW = pow(max(0.0, 1.0 - abs(yDist) * spikeLengthNW), 4.0);
        float spikeSE = pow(max(0.0, 1.0 - abs(xDist) * spikeLengthSE), 4.0);
        float spikeSW = pow(max(0.0, 1.0 - abs(yDist) * spikeLengthSW), 4.0);
        float spikes = (spikeNE + spikeNW + spikeSE + spikeSW);
         */
        
        float spikeLength = mix(400.0, 100.0, sizeSelector);
        float spikeX = 2.0f * pow(max(0.0, 1.0 - abs(xDist) * spikeLength), 4.0);
        float spikeY = 2.0f * pow(max(0.0, 1.0 - abs(yDist) * spikeLength), 4.0);
        float spikes = spikeX + spikeY;

        float spikeArea = pow(cosAngle, 100000.0); // Leggermente più ampio
        
        // strength depending on h, which is star and spikes presence
        float individualStrength = pow(h, 4.0) * 1.5;
        
        totalSpikes = spikes * spikeArea * individualStrength;
    }
    
    float3 baseColor = getStarColor(offset.y);
    
    // star: core (white) + colored glow
    float3 starSample = (core + (glow + totalSpikes) * baseColor) * brightness;
    return starSample;
}

fragment float4 fragmentSkybox(
    VertexOut vertexOut [[stage_in]],
    constant float4x4& inverseViewMatrix [[buffer(26)]],
    constant float4x4& inverseProjectionMatrix [[buffer(27)]],
    constant float4& cameraPosition [[buffer(28)]],
    constant ViewportSize &viewportSize [[buffer(29)]]
) {
    // ndc coordinates (viewport size required)
    float2 ndc = float2(vertexOut.position.x / viewportSize.size.x * 2.0f - 1.0f, -vertexOut.position.y / viewportSize.size.y * 2.0f + 1.0f);
    // get world position of the pixel with far z (z = 1.0)
    float4 farWorldPosition = inverseViewMatrix * inverseProjectionMatrix * float4(ndc, 1.0f, 1.0f);
    farWorldPosition /= farWorldPosition.w;
    // get direction from camera towards the point in the sky that is being rendered in this pixel
    float3 dir = normalize(farWorldPosition.xyz - cameraPosition.xyz);
    
    /*
    // fixed rotation for avoiding grid appearance (vertical and horizontal lines)
    float3x3 rot = float3x3(
        float3( 0.80,  0.60,  0.00),
        float3(-0.60,  0.80,  0.00),
        float3( 0.00,  0.00,  1.00)
    );

    // Applichiamo la rotazione al raggio di vista
    dir = normalize(rot * dir);*/

    // density: length of the direction vector;
    // it determines the resolution of the noise
    float density = 50.0;
    float3 gridPos = dir * density;
    
    // min vertex of the cube reached by this vector
    float3 baseId = floor(gridPos);

    float3 totalStar = float3(0.0);
    bool useNeighbors = true;
    float starsFraction = 0.95;
    // pixel get the light from its neighbors
    if (useNeighbors) {
        totalStar = float3(0.0);
        for (int x = -1; x <= 1; x++) {
            for (int y = -1; y <= 1; y++) {
                for (int z = -1; z <= 1; z++) {
                    float3 neighborId = baseId + float3(x, y, z);
                    totalStar += getPixelColor(neighborId, dir, starsFraction);
                }
            }
        }
    } else {
        totalStar = getPixelColor(baseId, dir, starsFraction);
    }
    
    float3 totalColor = totalStar;
    
    /*
    for (int i = 1; i < 5; i++) {
        // 1. Definiamo due vettori per spostare lo spazio
        float3 warpSeedA = float3(123.45, 67.89, 12.34);
        float3 warpSeedB = float3(-98.76, 54.32, -10.98);

        // 2. Creiamo un rumore a bassa frequenza per "warpare" (distorcere)
        // Alza la scala per rendere la distorsione più grande
        float3 warpRd = rotatedDir * 1 * i * i; // o rayDirection, se non le vuoi allineate

        // Otteniamo una distorsione per ogni asse (moltiplicata per l'hash per variare)
        float3 warp = float3(
            smoothNoise(warpRd + warpSeedA), // Solo 8 hash invece di 32
            smoothNoise(warpRd + warpSeedB),
            smoothNoise(warpRd + warpSeedA.zyx)
        );

        // 3. Campioniamo il colore con il raggio distorto
        // "Distorsione Potenziata": warp * 2.5 spinge la nebulosa a frastagliarsi
        float3 samplePos = warpRd + warp * 2.5 * i * i;
        // 1. Calcoliamo il rumore della nebulosa
        // Usiamo il rayDirection originale (non ruotato) per "staccarla" dalle stelle
        //float3 nebulaPos = rayDirection * 1.5 * i; // Scala bassa = nubi grandi e maestose
        float n = fbm(samplePos);
        
        // 2. CREAZIONE DEL COLORE E PROFONDITÀ
        // Usiamo un contrasto che non uccida i valori medi
        // float intensityFactor = 2.0f;
        float intensityFactor = pow(i, 0.5) / 2.0;
        float intensity = pow(n, 3.0) * intensityFactor; // Alziamo l'intensità globale
        
        float colorSelector = n;
        colorSelector = pow(colorSelector, 1.0);
        float3 col;
        if (colorSelector < 0.4) {
            col = mix(float3(0.02, 0.05, 0.2), float3(0.1, 0.2, 0.4), colorSelector / 0.4);
        } else if (colorSelector < 0.7) {
            col = mix(float3(0.1, 0.2, 0.4), float3(0.3, 0.1, 0.4), (colorSelector - 0.4) / 0.3);
        } else {
            col = mix(float3(0.3, 0.1, 0.4), float3(0.5, 0.1, 0.3), (colorSelector - 0.7) / 0.3);
        }
        col = col / 3.0;
        
        // Mescoliamo i colori in base al valore del rumore stesso
        float3 nebulaSample = col * intensity;
        
        // 3. IL TOCCO FINALE: "Luce Galattica"
        // Aggiungiamo un'ulteriore spinta luminosa solo dove la nube è densa
        nebulaSample += float3(0.2, 0.2, 0.3) * pow(n, 10);
        
        // Sommiamo al totale delle stelle
        totalColor += nebulaSample;
    }
     */
    return float4(totalColor, 1.0f);
    //return float4(0, 0, 0, 1);
}
