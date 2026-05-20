#include <metal_stdlib>
using namespace metal;

struct MeshUniforms {
    float4x4 modelMatrix;
    float4x4 viewMatrix;
    float4x4 projectionMatrix;
};

struct VertexOut {
    float4 position [[position]];
    float3 normal;
    float4 color;
};

struct PrimitiveOut {
    // No extra primitive data needed for basic rendering
};

// Icosahedron constants
[[mesh]]
void icosphere_mesh_shader(
    uint tid [[thread_index_in_threadgroup]],
    uint gid [[threadgroup_position_in_grid]],
    mesh<VertexOut, PrimitiveOut, 64, 64, topology::triangle> output,
    constant float4x4& modelMatrix [[buffer(28)]],
    constant float4x4& viewMatrix [[buffer(29)]],
    constant float4x4& projectionMatrix [[buffer(30)]]
) {
    // Icosahedron constants
    const float3 icosahedronVertices[12] = {
        normalize(float3(-1,  1.61803398875,  0)),
        normalize(float3( 1,  1.61803398875,  0)),
        normalize(float3(-1, -1.61803398875,  0)),
        normalize(float3( 1, -1.61803398875,  0)),
        normalize(float3( 0, -1,  1.61803398875)),
        normalize(float3( 0,  1,  1.61803398875)),
        normalize(float3( 0, -1, -1.61803398875)),
        normalize(float3( 0,  1, -1.61803398875)),
        normalize(float3( 1.61803398875,  0, -1)),
        normalize(float3( 1.61803398875,  0,  1)),
        normalize(float3(-1.61803398875,  0, -1)),
        normalize(float3(-1.61803398875,  0,  1))
    };

    const uint3 icosahedronIndices[20] = {
        uint3(0, 11, 5), uint3(0, 5, 1), uint3(0, 1, 7), uint3(0, 7, 10), uint3(0, 10, 11),
        uint3(1, 5, 9), uint3(5, 11, 4), uint3(11, 10, 2), uint3(10, 7, 6), uint3(7, 1, 8),
        uint3(3, 9, 4), uint3(3, 4, 2), uint3(3, 2, 6), uint3(3, 6, 8), uint3(3, 8, 9),
        uint3(4, 9, 5), uint3(2, 4, 11), uint3(6, 2, 10), uint3(8, 6, 7), uint3(9, 8, 1)
    };

    // Each threadgroup (gid) handles one face of the icosahedron
    if (gid >= 20) return;

    uint3 baseIndices = icosahedronIndices[gid];
    float3 v0 = icosahedronVertices[baseIndices.x];
    float3 v1 = icosahedronVertices[baseIndices.y];
    float3 v2 = icosahedronVertices[baseIndices.z];

    // Subdivision Level 2 (4 subdivisions per edge)
    // Vertices per edge = 5 (0, 1, 2, 3, 4)
    // Total vertices = (n+1)*(n+2)/2 = 5*6/2 = 15
    // Total triangles = n*n = 4*4 = 16
    
    const uint subdLevel = 2;
    const uint n = pow(2.0, subdLevel);
    const uint numVertices = (n + 1) * (n + 2) / 2;
    const uint numTriangles = n * n;

    if (tid == 0) {
        output.set_primitive_count(numTriangles);
    }

    // Vertex generation
    if (tid < numVertices) {
        // Calculate barycentric coordinates for the vertex
        // i: row, j: column in row
        uint i = 0;
        uint temp = tid;
        for (uint row = 0; row <= n; ++row) {
            uint verticesInRow = n + 1 - row;
            if (temp < verticesInRow) {
                i = row;
                break;
            }
            temp -= verticesInRow;
        }
        uint j = temp;

        float w = (float)i / n;
        float u = (float)j / n;
        float v = 1.0 - w - u;

        float3 p = normalize(v0 * v + v1 * u + v2 * w);
        
        VertexOut vout;
        float4 pos = float4(p, 1.0);
        vout.position = projectionMatrix * viewMatrix * modelMatrix * pos;
        vout.normal = p; // Unit sphere normal is the position
        vout.color = float4(p * 0.5 + 0.5, 1.0);
        
        output.set_vertex(tid, vout);
    }

    // Index generation
    if (tid < numTriangles) {
        // Find which triangle we are
        // This is a bit tricky to map tid to i, j and orientation
        // For a grid of n*n triangles:
        // There are n rows of triangles.
        // Row r (0 to n-1) has 2*(n-r)-1 triangles.
        
        uint r = 0;
        uint temp = tid;
        for (uint row = 0; row < n; ++row) {
            uint trianglesInRow = 2 * (n - row) - 1;
            if (temp < trianglesInRow) {
                r = row;
                break;
            }
            temp -= trianglesInRow;
        }
        uint c = temp;

        uint v_start_row = 0;
        for(uint row = 0; row < r; ++row) v_start_row += (n + 1 - row);
        uint v_next_row = v_start_row + (n + 1 - r);

        if (c % 2 == 0) {
            // Upward triangle
            uint j = c / 2;
            uint i0 = v_start_row + j;
            uint i1 = v_start_row + j + 1;
            uint i2 = v_next_row + j;
            output.set_index(tid * 3 + 0, i0);
            output.set_index(tid * 3 + 1, i1);
            output.set_index(tid * 3 + 2, i2);
        } else {
            // Downward triangle
            uint j = c / 2;
            uint i0 = v_start_row + j + 1;
            uint i1 = v_next_row + j + 1;
            uint i2 = v_next_row + j;
            output.set_index(tid * 3 + 0, i0);
            output.set_index(tid * 3 + 1, i1);
            output.set_index(tid * 3 + 2, i2);
        }
    }
}

[[fragment]]
float4 icosphere_fragment_shader(VertexOut in [[stage_in]]) {
    return float4(0.5, 0.5, 0.5, 1.0);
}
