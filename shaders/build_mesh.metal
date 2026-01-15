#include <metal_stdlib>
using namespace metal;

// Metal compute shader to build the same mesh produced by Mesh::fromPlanet
// - Replicates the BSpline basis and derivative calculations used by Planet
// - Produces positions, normals (via derivatives), texcoords and indices
//
// Buffer bindings (example suggested layout):
// buffer(0) -> device const float3* controlPoints (flattened parallels rows)
// buffer(1) -> device const int* knotsU
// buffer(2) -> device const int* knotsV
// buffer(3) -> device float3* outPositions
// buffer(4) -> device float3* outNormals
// buffer(5) -> device float2* outTexcoords
// buffer(6) -> device uint* outIndices
// buffer(7) -> Params (constant)
// NOTE: You can reorder bindings in the client as needed but adapt kernel signatures.

// Limits
constant int MAX_DEGREE = 3;

struct Params {
    int nU;
    int nV;
    int degreeU;
    int degreeV;
    int parallelsCount; // number of parallels rows (size of _parallels)
    int controlPointsPerParallel; // number of control points per parallel (row length)
    int knotsUSize;
    int knotsVSize;
    //int padding; // align to 32 bytes if needed
    float samplingRes; // optional recording
};

inline float toU(float t, const device int* knots, int knotsSize, int degree) {
    float a = (float)knots[degree - 1];
    float b = (float)knots[knotsSize - degree];
    return (1.0f - t) * a + t * b;
}

inline int span(float t, const device int* knots, int knotsSize, int degree) {
    float u = toU(t, knots, knotsSize, degree);
    int s = degree - 1;
    // protect bounds
    int limit = knotsSize - degree - 1;
    while (s < limit && u >= (float)knots[s + 1]) {
        s += 1;
    }
    return s;
}

// compute basis functions (degree + 1) into outBasis array (length at least degree+1)
inline void basisFunctions(int spanIdx, float t, const device int* knots, int knotsSize, int degree, thread float* outBasis) {
    float u = toU(t, knots, knotsSize, degree);
    // local arrays
    thread float basis[MAX_DEGREE + 1];
    thread float oldBasis[MAX_DEGREE + 1];
    // initialize
    for (int i = 0; i <= degree; ++i) {
        basis[i] = 0.0f;
        oldBasis[i] = 0.0f;
    }
    basis[degree] = 1.0f;
    oldBasis[degree] = 1.0f;

    for (int n = 1; n <= degree; ++n) {
        for (int j = 0; j <= degree; ++j) {
            int b = j + spanIdx - degree + 1;
            float w1 = 0.0f;
            float w2 = 0.0f;
            if (j > 0) {
                int d1 = knots[b + n - 1] - knots[b - 1];
                if (d1 != 0) w1 = (u - (float)knots[b - 1]) / (float)d1;
            }
            if (j < degree) {
                int d2 = knots[b + n] - knots[b];
                if (d2 != 0) w2 = ((float)knots[b + n] - u) / (float)d2;
            }
            float val = w1 * oldBasis[j];
            if (j < degree) val += w2 * oldBasis[j + 1];
            basis[j] = val;
        }
        // copy
        for (int k = 0; k <= degree; ++k) oldBasis[k] = basis[k];
    }
    // return: copy to outBasis
    for (int k = 0; k <= degree; ++k) outBasis[k] = basis[k];
}

// compute first derivative basis (d1Basis) of length degree+1
inline void d1BasisFunctions(int spanIdx, float t, const device int* knots, int knotsSize, int degree, thread float* outBasis) {
    float u = toU(t, knots, knotsSize, degree);
    thread float basis[MAX_DEGREE + 1];
    thread float oldBasis[MAX_DEGREE + 1];
    for (int i = 0; i <= degree; ++i) { basis[i] = 0.0f; oldBasis[i] = 0.0f; }
    basis[degree] = 1.0f; oldBasis[degree] = 1.0f;

    // compute up to degree-1
    for (int n = 1; n < degree; ++n) {
        for (int j = 0; j <= degree; ++j) {
            int b = j + spanIdx - degree + 1;
            float w1 = 0.0f;
            float w2 = 0.0f;
            if (j > 0) {
                int d1 = knots[b + n - 1] - knots[b - 1];
                if (d1 != 0) w1 = (u - (float)knots[b - 1]) / (float)d1;
            }
            if (j < degree) {
                int d2 = knots[b + n] - knots[b];
                if (d2 != 0) w2 = ((float)knots[b + n] - u) / (float)d2;
            }
            float val = w1 * oldBasis[j];
            if (j < degree) val += w2 * oldBasis[j + 1];
            basis[j] = val;
        }
        for (int k = 0; k <= degree; ++k) oldBasis[k] = basis[k];
    }
    // compute derivative from last step
    for (int j = 0; j <= degree; ++j) {
        int b = j + spanIdx - degree + 1;
        float w1 = 0.0f;
        float w2 = 0.0f;
        if (j > 0) {
            int d1 = knots[b + degree - 1] - knots[b - 1];
            if (d1 != 0) w1 = (float)degree / (float)d1;
        }
        if (j < degree) {
            int d2 = knots[b + degree] - knots[b];
            if (d2 != 0) w2 = - (float)degree / (float)d2;
        }
        float val = w1 * oldBasis[j];
        if (j < degree) val += w2 * oldBasis[j + 1];
        outBasis[j] = val;
    }
}

// Access control point at paralle lrow p and column q
inline float3 cpAt(const device packed_float3* controlPoints, int parallelsCount, int cpPerParallel, int p, int q) {
    // wrap safety
    int r = p;
    int c = q;
    packed_float3 v = controlPoints[r * cpPerParallel + c];
    return float3(v.x, v.y, v.z);
}

// Evaluate Planet::evaluate(u,v) using BSpline bases. Returns position and optionally derivatives
inline float3 evaluatePlanetPosition(const device packed_float3* controlPoints,
                                     const device int* knotsU, int knotsUSize,
                                     const device int* knotsV, int knotsVSize,
                                     constant Params& params,
                                     float u, float v)
{
    int spanU = span(u, knotsU, knotsUSize, params.degreeU);
    int spanV = span(v, knotsV, knotsVSize, params.degreeV);

    thread float basisU[MAX_DEGREE + 1];
    thread float basisV[MAX_DEGREE + 1];
    basisFunctions(spanU, u, knotsU, knotsUSize, params.degreeU, basisU);
    basisFunctions(spanV, v, knotsV, knotsVSize, params.degreeV, basisV);

    float3 result = float3(0.0);
    for (int i = 0; i <= params.degreeV; ++i) {
        for (int j = 0; j <= params.degreeU; ++j) {
            int row = spanV - params.degreeV + 1 + i;
            int col = spanU - params.degreeU + 1 + j;
            float3 P = cpAt(controlPoints, params.parallelsCount, params.controlPointsPerParallel, row, col);
            result += basisU[j] * basisV[i] * P;
        }
    }
    return result;
}

inline float3 evaluatePlanetUFirstDerivative(const device packed_float3* controlPoints,
                                             const device int* knotsU, int knotsUSize,
                                             const device int* knotsV, int knotsVSize,
                                             constant Params& params,
                                             float u, float v)
{
    int spanU = span(u, knotsU, knotsUSize, params.degreeU);
    int spanV = span(v, knotsV, knotsVSize, params.degreeV);
    thread float dBasisU[MAX_DEGREE + 1];
    thread float basisV[MAX_DEGREE + 1];
    d1BasisFunctions(spanU, u, knotsU, knotsUSize, params.degreeU, dBasisU);
    basisFunctions(spanV, v, knotsV, knotsVSize, params.degreeV, basisV);

    float3 result = float3(0.0);
    for (int i = 0; i <= params.degreeV; ++i) {
        for (int j = 0; j <= params.degreeU; ++j) {
            int row = spanV - params.degreeV + 1 + i;
            int col = spanU - params.degreeU + 1 + j;
            float3 P = cpAt(controlPoints, params.parallelsCount, params.controlPointsPerParallel, row, col);
            result += dBasisU[j] * basisV[i] * P;
        }
    }
    return result;
}

inline float3 evaluatePlanetVFirstDerivative(const device packed_float3* controlPoints,
                                             const device int* knotsU, int knotsUSize,
                                             const device int* knotsV, int knotsVSize,
                                             constant Params& params,
                                             float u, float v)
{
    int spanU = span(u, knotsU, knotsUSize, params.degreeU);
    int spanV = span(v, knotsV, knotsVSize, params.degreeV);
    thread float basisU[MAX_DEGREE + 1];
    thread float dBasisV[MAX_DEGREE + 1];
    basisFunctions(spanU, u, knotsU, knotsUSize, params.degreeU, basisU);
    d1BasisFunctions(spanV, v, knotsV, knotsVSize, params.degreeV, dBasisV);

    float3 result = float3(0.0);
    for (int i = 0; i <= params.degreeV; ++i) {
        for (int j = 0; j <= params.degreeU; ++j) {
            int row = spanV - params.degreeV + 1 + i;
            int col = spanU - params.degreeU + 1 + j;
            float3 P = cpAt(controlPoints, params.parallelsCount, params.controlPointsPerParallel, row, col);
            result += basisU[j] * dBasisV[i] * P;
        }
    }
    return result;
}


// Kernel to build vertex buffers: positions, normals (via derivatives), texcoords
kernel void buildVertices(
    device const packed_float3* controlPoints [[ buffer(0) ]],
    device const int* knotsU [[ buffer(1) ]],
    device const int* knotsV [[ buffer(2) ]],
    device packed_float3* outPositions [[ buffer(3) ]],
    device packed_float3* outNormals [[ buffer(4) ]],
    device packed_float2* outTexcoords [[ buffer(5) ]],
    constant Params& params [[ buffer(7) ]],
    uint gid [[ thread_position_in_grid ]])
{
    int nU = params.nU;
    int nV = params.nV;

    // compute total vertices as in Mesh::fromPlanet
    int innerRows = max(0, nV - 4); // clamp to avoid negative counts
    int numVertices = 1 + (innerRows * nU) + 1; // south pole + inner + north pole
    if (gid >= (uint)numVertices) return;

    if (gid == 0) {
        // south pole evaluate(0,0)
        float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 0.0f);
        outPositions[0] = pos;
        // normal via derivative
        float3 du = evaluatePlanetUFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 0.0f);
        float3 dv = evaluatePlanetVFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 0.0f);
        float3 n0 = normalize(cross(du, dv));
        outNormals[0] = packed_float3(n0.x, n0.y, n0.z);
        outTexcoords[0] = packed_float2(0.0, 0.0);
        return;
    }
    int lastIdx = numVertices - 1;
    if (gid == (uint)lastIdx) {
        float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 1.0f);
        outPositions[lastIdx] = pos;
        float3 du = evaluatePlanetUFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 1.0f);
        float3 dv = evaluatePlanetVFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 1.0f);
        float3 nLast = normalize(cross(du, dv));
        outNormals[lastIdx] = packed_float3(nLast.x, nLast.y, nLast.z);
        outTexcoords[lastIdx] = packed_float2(0.0, 1.0);
        return;
    }

    // inner points
    int k = int(gid) - 1;
    int row = k / nU; // 0..innerRows-1
    int col = k % nU; // 0..nU-1
    int i = row + (1 + 1); // matches CPU: i from 2..nV-3
    int j = col;
    float fu = static_cast<float>(j) / static_cast<float>(nU);
    float fv = static_cast<float>(i) / static_cast<float>(nV - 1);
    float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, fu, fv);
    outPositions[gid] = packed_float3(pos.x, pos.y, pos.z);
    float3 du = evaluatePlanetUFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, fu, fv);
    float3 dv = evaluatePlanetVFirstDerivative(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, fu, fv);
    float3 n = normalize(cross(du, dv));
    outNormals[gid] = packed_float3(n.x, n.y, n.z);
    outTexcoords[gid] = packed_float2(fu, fv);
}

// Kernel to build vertex buffers: positions, normals (via derivatives), texcoords
kernel void buildVerticesOnlyPosition(
    device const packed_float3* controlPoints [[ buffer(0) ]],
    device const int* knotsU [[ buffer(1) ]],
    device const int* knotsV [[ buffer(2) ]],
    device packed_float3* outPositions [[ buffer(3) ]],
    constant Params& params [[ buffer(7) ]],
    uint gid [[ thread_position_in_grid ]])
{
    int nU = params.nU;
    int nV = params.nV;
    
    // compute total vertices as in Mesh::fromPlanet
    int innerRows = max(0, nV - 4); // clamp to avoid negative counts
    int numVertices = 1 + (innerRows * nU) + 1; // south pole + inner + north pole
    if (gid >= (uint)numVertices) return;
    
    if (gid == 0u) {
        // south pole evaluate(0,0)
        float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 0.0f);
        outPositions[0] = packed_float3(pos.x, pos.y, pos.z);
        return;
    }
    int lastIdx = numVertices - 1;
    
    if (gid == (uint)lastIdx) {
        float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, 0.0f, 1.0f);
        outPositions[lastIdx] = packed_float3(pos.x, pos.y, pos.z);
        return;
    }
    
    // inner points
    int k = int(gid) - 1;
    int row = k / nU; // 0..innerRows-1
    int col = k % nU; // 0..nU-1
    int i = row + (1 + 1); // matches CPU: i from 2..nV-3
    int j = col;
    float fu = static_cast<float>(j) / static_cast<float>(nU);
    float fv = static_cast<float>(i) / static_cast<float>(nV - 1);
    float3 pos = evaluatePlanetPosition(controlPoints, knotsU, params.knotsUSize, knotsV, params.knotsVSize, params, fu, fv);
    outPositions[gid] = packed_float3(pos.x, pos.y, pos.z);
}

// Kernel to build indices (triangles) following same ordering as Mesh::fromPlanet
kernel void buildIndices(
    device uint* outIndices [[ buffer(6) ]],
    constant Params& params [[ buffer(7) ]],
    uint gid [[ thread_position_in_grid ]])
{
    int nU = params.nU;
    int nV = params.nV;

    // Compute counts using same logic as CPU
    int innerRows = max(0, nV - 4); // nV - 4 clamped
    int numVertices = 1 + (innerRows * nU) + 1;

    // Triangles count: compute same sequence to match CPU
    // First fan around south pole: nU triangles
    int idx = 0;
    // We'll let each thread write multiple indices; better map gid to triangle index
    // Compute total triangles by emulating CPU loops
    int totalTriangles = 0;
    // south pole fan
    totalTriangles += nU;
    // inner parallels (clamp to zero so negative values don't produce huge unsigned casts)
    int innerTriRows = max(0, nV - 5); // equals nV - 5 clamped
    if (innerTriRows > 0) {
        totalTriangles += innerTriRows * nU * 2; // two triangles per cell
    }
    // north fan
    totalTriangles += nU;

    // If gid corresponds to triangle index, map to write
    int triIndex = int(gid);
    if (triIndex >= totalTriangles) return;

    // We'll write indices as flat: each triangle uses 3 uints, so offset = triIndex*3
    int outOffset = triIndex * 3;

    // Determine which region triIndex belongs to
    if (triIndex < nU) {
        // south pole fan (note: CPU labels it NORTH POLE FAN but uses southPoleIdx=0)
        uint32_t southPoleIdx = 0u;
        uint32_t v1 = 1u + (uint32_t)triIndex;
        uint32_t v2 = 1u + (uint32_t)((triIndex + 1) % nU);
        outIndices[outOffset + 0] = southPoleIdx;
        outIndices[outOffset + 1] = v1;
        outIndices[outOffset + 2] = v2;
        return;
    }
    int consumed = nU;
    // inner parallels
    int innerRowsCPU = max(0, nV - 5); // nV -5 clamped
    if (innerRowsCPU > 0) {
        int innerTotal = innerRowsCPU * nU * 2;
        if (triIndex < consumed + innerTotal) {
            int local = triIndex - consumed; // 0..innerTotal-1
            int cell = local / 2; // 0..innerRowsCPU*nU -1
            int triInCell = local % 2; // 0 or 1
            int cellRow = cell / nU; // 0..innerRowsCPU-1
            int cellCol = cell % nU; // 0..nU-1
            // map to vertex indices as CPU does:
            uint32_t row0 = 1u + (uint32_t)(cellRow) * (uint32_t)nU;
            uint32_t row1 = 1u + (uint32_t)(cellRow + 1) * (uint32_t)nU;
            uint32_t v0 = row0 + (uint32_t)cellCol;
            uint32_t v1 = row0 + (uint32_t)((cellCol + 1) % nU);
            uint32_t v2 = row1 + (uint32_t)cellCol;
            uint32_t v3 = row1 + (uint32_t)((cellCol + 1) % nU);
            if (triInCell == 0) {
                // First triangle: v0, v2, v1
                outIndices[outOffset + 0] = v0;
                outIndices[outOffset + 1] = v2;
                outIndices[outOffset + 2] = v1;
            } else {
                // Second triangle: v1, v2, v3
                outIndices[outOffset + 0] = v1;
                outIndices[outOffset + 1] = v2;
                outIndices[outOffset + 2] = v3;
            }
            return;
        }
        consumed += innerTotal;
    }
    // north fan (remaining)
    int localIndex = triIndex - consumed; // 0..nU-1
    // compute northPoleIdx and lastRow as CPU
    uint32_t northPoleIdx = (uint32_t)(numVertices - 1);
    // lastRow corresponds to the start of the last inner ring: 1 + (innerRows - 1) * nU
    uint32_t lastRow = 1u + (uint32_t)(max(0, innerRows - 1)) * (uint32_t)nU;
    uint32_t v1 = lastRow + (uint32_t)localIndex;
    uint32_t v2 = lastRow + (uint32_t)((localIndex + 1) % nU);
    outIndices[outOffset + 0] = v1;
    outIndices[outOffset + 1] = northPoleIdx;
    outIndices[outOffset + 2] = v2;
}
