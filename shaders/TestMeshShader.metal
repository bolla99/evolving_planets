#include <metal_stdlib>
using namespace metal;

#include "Lighting.hpp"

// Vertex output structure
struct VertexOut {
    float4 position [[position]];
    float4 worldPosition;
    float4 normal;
    float4 color;
};

// Mesh shader output primitive structure
struct PrimitiveOut {
    // Empty for now
};

// Simple mesh shader that passes through icosphere data
using MeshType = metal::mesh<VertexOut, PrimitiveOut, 128, 126, metal::topology::triangle>;

// Buffer Layout:
// The RenderableFactory creates buffers in this order based on mesh attributes:
// buffer(0) = first attribute (typically Position - float4, padded from float3)
// buffer(1) = second attribute (typically Normal - float4, padded from float3)
// buffer(2) = third attribute (typically Color - float4)
// buffer(N) = indices (uint) - at slot N where N = number of attributes
// buffer(27-30) = transformation matrices (from materials)

[[mesh]]
void testMeshShader(
    uint threadgroup_id [[threadgroup_position_in_grid]],
    uint thread_id [[thread_position_in_threadgroup]],
    MeshType output,
    device const float3* positions [[buffer(0)]],
    device const float4* colors [[buffer(1)]],
    device const float3* normals [[buffer(2)]],
    device const uint* indices [[buffer(3)]],
    device const uint& numIndices [[buffer(4)]],
    constant float3x3& normalMatrix [[buffer(27)]],
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
) {
    const uint trianglesPerMeshlet = 32;
    const uint verticesPerMeshlet = trianglesPerMeshlet * 3;
    const uint baseIndex = threadgroup_id * verticesPerMeshlet;
    output.set_primitive_count(trianglesPerMeshlet);
     

    for (uint i = thread_id; i < verticesPerMeshlet; i += 32) {
        uint globalIdxPos = baseIndex + i;
        
        // Invece di 'continue', usiamo un indice sicuro (0) se siamo fuori bordo
        //bool isValid = (globalIdxPos < numIndices);
        //uint vertexLookup = isValid ? indices[globalIdxPos] : indices[0];
        uint id = indices[globalIdxPos];
        float3 pos = positions[id];
        float3 norm = normals[id];
        float4 col = colors[id];

        VertexOut vo;
        float4 worldPos = modelMatrix * float4(pos, 1.0);
        vo.position = projectionMatrix * viewMatrix * worldPos;
        
        // Se il vertice è invalido, lo "nascondiamo" (opzionale, il count delle primitive già aiuta)
        //if (!isValid) vo.position = float4(0,0,0,0);

        vo.worldPosition = worldPos;
        vo.normal = float4(normalMatrix * norm, 0.0f);
        vo.color = col;

        output.set_vertex(i, vo);
    }

    // 3. Definizione Indici (Topologia locale)
    // Usiamo tutti i thread per settare gli indici in parallelo
    for (uint i = thread_id; i < trianglesPerMeshlet; i += 32) {
        uint globalTriPos = baseIndex + (i * 3);
        if (globalTriPos + 2 < numIndices) {
            output.set_index(i * 3 + 0, i * 3 + 0);
            output.set_index(i * 3 + 1, i * 3 + 1);
            output.set_index(i * 3 + 2, i * 3 + 2);
        }
    }
}

// Fragment shader - reuse existing Phong lighting
fragment float4 testMeshFragment(
    VertexOut vertexOut [[stage_in]]
    //constant float& shininess [[buffer(26)]],
    //constant float4& cameraPosition [[buffer(27)]],
    //constant Lights& lights [[buffer(28)]]
) {
    //float4 position = vertexOut.worldPosition;
    //float4 color = vertexOut.color;
    return vertexOut.color;
    //float4 normal = normalize(vertexOut.normal);

    //return applyPHONGLights(position, color, normal, shininess, cameraPosition, lights);
}
