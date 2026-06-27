#include <metal_stdlib>
#include "util.hpp"
#include "octree.hpp"
#include "bvh.hpp"
#include "rayleigh.hpp"
#include "Lighting.hpp"

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
    float4 currentClipPosition;
    float4 previousClipPosition;
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
    constant ViewportSize &viewportSize [[buffer(3)]],
    constant float2& jitter [[buffer(4)]],
    constant float4x4& inverseViewProjectionMatrix [[buffer(5)]],
    constant float4x4& viewProjectionMatrix [[buffer(6)]],
    constant float4x4& previousViewProjectionMatrix [[buffer(7)]],
    constant float4& cameraPosition [[buffer(8)]],
    constant float& TAAScaling [[buffer(9)]],
    constant float4x4& projectionMatrix [[buffer(10)]],
    constant float4x4& inverseProjectionMatrix [[buffer(11)]]
) {
    VertexOut vOut;
    float4 currentPosition;
    currentPosition.xy = vertexIn.position.xy * rect.size + rect.position;
    currentPosition.z = vertexIn.position.z;
    currentPosition.w = 1.0;
    auto vs = floor(viewportSize.size * TAAScaling);
    currentPosition.x = currentPosition.x / vs.x * 2.0f - 1.0f;
    currentPosition.y = -currentPosition.y / vs.y * 2.0f + 1.0f;
    
    vOut.currentClipPosition = currentPosition;
    vOut.position = currentPosition;
    //apply jitter
    vOut.position.xy -= jitter;
    
    // 1. Spazio Mondo Omogeneo (lineare, NESSUNA divisione per W)
    float4 worldPos = inverseViewProjectionMatrix * currentPosition;
    auto currentViewMatrix = inverseProjectionMatrix * viewProjectionMatrix;
    auto previousViewMatrix = inverseProjectionMatrix * previousViewProjectionMatrix;
    previousViewMatrix[3][0] = currentViewMatrix[3][0];
    previousViewMatrix[3][1] = currentViewMatrix[3][1];
    previousViewMatrix[3][2] = currentViewMatrix[3][2];
    previousViewMatrix[3][3] = currentViewMatrix[3][3];
    vOut.previousClipPosition = projectionMatrix * previousViewMatrix * worldPos;

        
    return vOut;
}

inline float3 getPixelColor(float3 id, float3 dir, float starsFraction) {
    // h is the probability that in that cube a star is present
    float h = hash1d(id * 12.9898 + id.yzx * 78.233);
    
    // if h > starsFraction then the star is present
    if (h < starsFraction) return float3(0.0, 0.0, 0.0);
    
    // star position inside the cube
    float3 offset = hash33(id);
        
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

struct FragmentOut {
    float4 color [[color(0)]];        // Colore visibile a schermo
    float2 motionVector [[color(1)]]; // La nostra texture RG16Float
};

fragment FragmentOut fragmentSkybox(
    VertexOut vertexOut [[stage_in]],
    constant float4x4& inverseViewMatrix [[buffer(26)]],
    constant float4x4& inverseProjectionMatrix [[buffer(27)]],
    constant float4& cameraPosition [[buffer(28)]],
    constant ViewportSize &viewportSize [[buffer(29)]],
    constant float4& sunDirection [[buffer(30)]],
    constant int* octree [[buffer(12)]],
    constant PotentialOctreeInfo& octreeInfo [[buffer(13)]],
    constant BVHNode* bvhNodes [[buffer(14)]],
    constant Triangle* bvhPrimitives [[buffer(15)]],
    constant BVHInfo* bvhInfo [[buffer(16)]],
    constant PotentialSamplingInfo& potentialSamplingInfo [[buffer(17)]],
    constant packed_float3& sunColor [[buffer(18)]],
    constant ShadowData& shadowData [[buffer(19)]],
    constant Lights& lights [[buffer(20)]],
    texture3d<float> densityTexture [[texture(0)]],
    texture3d<float> lightTransmittanceTexture [[texture(1)]],
    depth2d_array<float> shadowMaps [[texture(2)]],
    sampler densitySampler [[sampler(0)]],
    constant AtmosphereSettings& atmosphereSettings [[buffer(21)]]
                    
) {
    // get world position of the pixel with far z (z = 1.0)
    float4 farWorldPosition = inverseViewMatrix * inverseProjectionMatrix * float4(vertexOut.currentClipPosition);
    farWorldPosition /= farWorldPosition.w;
    // get direction from camera towards the point in the sky that is being rendered in this pixel
    float3 dir = normalize(farWorldPosition.xyz - cameraPosition.xyz);
    
    //auto jitter = interleavedGradientNoise(vertexOut.position.xy);
    auto jitter = smoothNoise(farWorldPosition.xyz * 10000.0);
    if (not atmosphereSettings.jitter) jitter = 0.5f;
    
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
    float starDensity = 50.0;
    float3 gridPos = dir * starDensity;
    
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
    
    
    // SUN
    auto dirTowardsSun = -sunDirection;
    auto dotSun = max(dot(normalize(dir), normalize(dirTowardsSun.xyz)), 0.0f);
    
    float sunCore = pow(dotSun, 100000.0f);
    float sunCorona = pow(dotSun, 5000.0f);
    float sunGlow = pow(dotSun, 400.0f);
    
    // 1. Il Nucleo: Bianco puro spinto al massimo
    //float3 coreColor = float3(30.0f, 30.0f, 30.0f) * sunCore;
    float3 coreColor = sunColor * sunCore;

        
    // 2. La Corona: Invece di farla gialla o azzurra, impostiamo i tre canali
    // quasi identici. (15, 15, 14.5) crea un bianco sporco impercettibile,
    // dando una transizione naturale senza viraggi di colore visibili.
    //float3 coronaColor = float3(15.0f, 15.0f, 14.5f) * sunCorona;
    float3 coronaColor = sunColor / 2.0f * sunCorona;

        
    // 3. Il Glow: L'alone sfumato finale non è né rosso né blu. Con (2.0, 1.9, 1.7)
    // ottieni un grigio-argento caldissimo (o una tonalità "perla") che si spegne
    // istantaneamente nel vuoto cosmico dello skybox.
    //float3 glowColor = float3(2.0f, 1.9f, 1.7f) * sunGlow;
    float3 glowColor = sunColor / 15.0f * sunGlow;

    
    // Combiniamo il tutto
    float3 finalSun = coreColor + coronaColor + glowColor;
    
    totalColor += finalSun;
    
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
    
    FragmentOut fOut;
    fOut.color = float4(totalColor, 1.0f);

    fOut.motionVector = motionVector(vertexOut.currentClipPosition, vertexOut.previousClipPosition);
    
    /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    
    float3 backgroundSpace = totalColor;
        
    float3 rayOrigin = cameraPosition.xyz; // Assicurati di usare il nome della tua variabile camera
    float3 rayDir = normalize(dir);           // La direzione del raggio che hai calcolato nello screenshot
    float3 L = normalize(-sunDirection.xyz); // La direzione VERSO il sole che usi già per il finto disco solare

    /*
    float3 planetCenter = float3(0.0f, 0.0f, 0.0f); // Modificalo se il pianeta si muove
    const float R_planet = 237.0f;       // Il raggio REALE del pianeta nel tuo engine
    const float Atmo_Thickness = 17.0f;  // Lo spessore effettivo dell'atmosfera (es. 254 - 237)
    const float R_atmo   = R_planet + Atmo_Thickness; // Raggio esterno dell'atmosfera (254.0f)

    const float H_R      = Atmo_Thickness * 0.12f; // Circa 2.04 unità (invece di 20)

    float3 betaR   = float3(0.148f, 0.344f, 0.844f);
    float3 I_sun   = float3(20.0f); // Intensità luce solare

    // Intersezione con i confini dell'atmosfera
    float t_start, t_exit;
    if (!raySphereIntersect(rayOrigin, rayDir, planetCenter, R_atmo, t_start, t_exit)) {
        // Se il raggio punta lo spazio profondo fuori dall'atmosfera, mostra lo skybox originale pulito
        return {float4(backgroundSpace, 1.0f), fOut.motionVector};
    }
    t_start = max(0.0f, t_start);

    // Intersezione con la superficie solida del pianeta sferico
    float tp0, tp1;
    float t_end = t_exit;
    if (raySphereIntersect(rayOrigin, rayDir, planetCenter, R_planet, tp0, tp1)) {
        if (tp0 > 0.0f) {
            t_end = min(t_end, tp0); // Il raggio si ferma sulla roccia del pianeta
        }
    }*/
    
    // --- 1. IL GUSCIO ESTERNO (Resta una sfera generosa che contiene tutto il sistema) ---
    float3 planetCenter = float3(0.0f, 0.0f, 0.0f);
    const float R_atmo = potentialSamplingInfo.nonZeroDensityRadius;
    float3 betaR   = float3(0.148f, 0.344f, 0.844f);
    float3 I_sun   = sunColor; // Intensità luce solare

    float t_start = 0.0;
    float t_exit = 0.0;
    if (!raySphereIntersect(rayOrigin, rayDir, planetCenter, R_atmo, t_start, t_exit)) {
        return {float4(backgroundSpace, 1.0f), fOut.motionVector};
    }
    t_start = max(0.0f, t_start);
    t_exit = max(0.0f, t_exit);
        
    float t_end = t_exit;

     
    // Loop di Raymarching dell'aria
    const int SAMPLES = atmosphereSettings.SAMPLES;
    const int SUN_SAMPLES = atmosphereSettings.SUN_SAMPLES;
    float stepLength = (t_end - t_start) / float(SAMPLES);
        
    float3 accumulatedScattering = float3(0.0f);
    float3 transmittanceCamera = float3(1.0f);

    // Funzione di fase di Rayleigh analitica
    float mu = dot(rayDir, L);
    float phaseR = 0.75f * (1.0f + mu * mu) / (4.0f * 3.14159265f);
    
    for (int i = 0; i < SAMPLES; i++) {
        float t = t_start + stepLength * (float(i) + jitter);
        float3 P = rayOrigin + rayDir * t;
        
        auto normalizedP = (P - potentialSamplingInfo.min.xyz) / potentialSamplingInfo.edge;
        
        float density = 0.0f;
        // potential texture now contains density
        if (all(normalizedP >= 0.0f) && all(normalizedP <= 1.0f)) {
            density = sampleDensitySmooth3D(densityTexture, densitySampler, normalizedP);
        }
        
        //float density = exp((Phi_surface - phi) / 100.0);
        float3 stepOpticalDepth = betaR * density * stepLength;
            
        float3 T_sun;
        // Trasmittanza dal sole a questo punto nello spazio
        if (not atmosphereSettings.useBakedLightTransmittance) {
            T_sun = lightTransmittance(P, L, potentialSamplingInfo, betaR,
                                              densityTexture, densitySampler, jitter, shadowMaps, shadowData, lights.numDirectionalLights, P, false, 1.0f, 0.01f, false, SUN_SAMPLES);
        } else {
            T_sun = lightTransmittanceFromTexture(P, potentialSamplingInfo, lightTransmittanceTexture, densitySampler, betaR);
        }
            
        // Accumulo in-scattering
        accumulatedScattering += T_sun * transmittanceCamera * (betaR * density * stepLength) * phaseR * I_sun;
            
        // Attenuazione della trasmittanza della camera (Legge di Beer-Lambert)
        transmittanceCamera *= exp(-stepOpticalDepth);
    }

    // --- BLENDING FINALE ---
    // Applichiamo l'effetto: lo sfondo viene spento dall'opacità dell'aria (transmittanceCamera)
    // e viene aggiunto il colore azzurro/arancione dell'atmosfera illuminata
    float3 finalColor = backgroundSpace * transmittanceCamera + accumulatedScattering;
    finalColor += (jitter - 0.5f) / 255.0f;
    return {float4(finalColor, 1.0f), fOut.motionVector};
    
    
    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    return fOut;
}
