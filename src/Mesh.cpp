//
// Created by Giovanni Bollati on 11/06/25.
//
#include "Mesh.hpp"
#include <BSpline.hpp>
#include <genetic.hpp>
#include <random>

#import <Metal/Metal.hpp>
#include <Foundation/Foundation.hpp>


Geometry::Mesh::Mesh(int numVertices, int numFaces,
     const std::vector<Core::VertexAttributeName>& vertexAttributeNames,
     const std::vector<Core::VertexAttributeType>& vertexAttributeTypes,
     const std::vector<std::vector<uint8_t>>& vertexData,
     const std::vector<uint32_t>& faces) : _numVertices(numVertices),
                                                            _numFaces(numFaces),
                                                            _vertexAttributeNames(vertexAttributeNames),
                                                            _vertexAttributeTypes(vertexAttributeTypes),
                                                            _vertexData(vertexData),
                                                            _faces(faces) {}

bool Geometry::Mesh::HasAttribute(Core::VertexAttributeName attributeName) const
{
    for (auto& name : _vertexAttributeNames)
    {
        if (name == attributeName)
        {
            return true;
        }
    }
    return false;
}

Core::VertexAttributeType Geometry::Mesh::GetAttributeType(Core::VertexAttributeName attributeName) const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == attributeName)
        {
            return _vertexAttributeTypes[i];
        }
    }
    throw std::runtime_error("Attribute not found");
}

void Geometry::Mesh::recalculateNormals()
{
    if (!HasAttribute(Core::VertexAttributeName::Normal))
    {
        return;
    }
    if (!HasAttribute(Core::VertexAttributeName::Position))
    {
        throw std::runtime_error("Mesh does not contain positions");
    }

    // Locate the normal attribute slot.
    size_t normalAttrIndex = 0;
    bool found = false;
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == Core::VertexAttributeName::Normal)
        {
            normalAttrIndex = i;
            found = true;
            break;
        }
    }
    if (!found)
    {
        return;
    }

    if (GetAttributeType(Core::VertexAttributeName::Normal) != Core::VertexAttributeType::Float3)
    {
        throw std::runtime_error("Mesh normal attribute must be Float3");
    }
    if (_vertexData[normalAttrIndex].size() != static_cast<size_t>(_numVertices) * 3 * sizeof(float))
    {
        throw std::runtime_error("Mesh normal buffer has unexpected size");
    }

    auto vertices = getVertices();
    std::vector<glm::vec3> normals(static_cast<size_t>(_numVertices), glm::vec3(0.0f));

    // Accumulate face normals based on winding order.
    // We trust the winding order is already correct (set by fromIcoPlanet/fromIcoPlanetRockyfied).
    for (size_t i = 0; i + 2 < _faces.size(); i += 3)
    {
        const uint32_t i0 = _faces[i + 0];
        const uint32_t i1 = _faces[i + 1];
        const uint32_t i2 = _faces[i + 2];

        if (i0 >= vertices.size() || i1 >= vertices.size() || i2 >= vertices.size())
        {
            continue;
        }

        const glm::vec3& p0 = vertices[i0];
        const glm::vec3& p1 = vertices[i1];
        const glm::vec3& p2 = vertices[i2];

        // Compute face normal from winding order.
        // Not normalized on purpose: this implicitly weights by triangle area.
        glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        const float nLen2 = glm::length2(n);
        if (nLen2 <= 1e-20f)
        {
            continue;
        }

        normals[i0] += n;
        normals[i1] += n;
        normals[i2] += n;
    }

    // Normalize vertex normals.
    for (size_t i = 0; i < normals.size(); ++i)
    {
        auto& n = normals[i];
        const float len2 = glm::length2(n);
        if (len2 > 1e-20f)
        {
            n *= glm::inversesqrt(len2);
        }
        else
        {
            // Fallback: radial direction from origin
            if (glm::length2(vertices[i]) > 1e-20f)
            {
                n = glm::normalize(vertices[i]);
            }
            else
            {
                n = glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }
    }

    std::memcpy(
        static_cast<void*>(_vertexData[normalAttrIndex].data()),
        static_cast<const void*>(normals.data()),
        _vertexData[normalAttrIndex].size()
    );
}

const std::vector<uint8_t>& Geometry::Mesh::getAttributeData(Core::VertexAttributeName attributeName) const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == attributeName)
        {
            return _vertexData[i];
        }
    }
    throw std::runtime_error("Attribute data not found");
}

const std::vector<uint32_t>& Geometry::Mesh::getFacesData() const
{
    return _faces;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::dummy()
{
    std::vector<Core::VertexAttributeName> attributeNames = {Core::VertexAttributeName::Position};
    std::vector<Core::VertexAttributeType> attributeTypes = {Core::VertexAttributeType::Float3};
    std::vector<std::vector<uint8_t>> vertexData(1);
    
    glm::vec3 dummyPos(0.0f);
    vertexData[0].resize(sizeof(glm::vec3));
    memcpy(vertexData[0].data(), &dummyPos, sizeof(glm::vec3));

    std::vector<uint32_t> indices = {0, 0, 0};

    return std::make_shared<Geometry::Mesh>(
        1,
        1,
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::quad(const glm::vec4& rect, float depth, const glm::vec4& color, float uvWidth, float uvHeight)
{
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::TexCoord
    };

    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float2
    };

    std::vector<std::vector<uint8_t>> vertexData(3);

    // Position data for 4 vertices
    std::vector<float> positions = {
        rect[0], rect[1], depth, // v0
        rect[0] + rect[2], rect[1], depth, // v1
        rect[0] + rect[2], rect[1] + rect[3], depth, // v2
        rect[0], rect[1] + rect[3], depth // v3
    };

    vertexData[0].resize(positions.size() * sizeof(float));
    std::memcpy(vertexData[0].data(), positions.data(), vertexData[0].size());

    // Color data for 4 vertices
    std::vector<float> colors = {
        color[0], color[1], color[2], color[3], // v0
        color[0], color[1], color[2], color[3], // v1
        color[0], color[1], color[2], color[3], // v2
        color[0], color[1], color[2], color[3] // v3
    };
    vertexData[1].resize(colors.size() * sizeof(float));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());

    if (uvWidth < 0.0f || uvWidth > 1.0f)
    {
        throw std::invalid_argument("uvWidth must be in the range [0.0, 1.0]");
    }

    std::vector<float> uvs = {
        0.0f, 0.0f, // v0
        uvWidth, 0.0f, // v1
        uvWidth, uvHeight, // v2
        0.0f, uvHeight // v3
    };

    vertexData[2].resize(uvs.size() * sizeof(float));
    std::memcpy(vertexData[2].data(), uvs.data(), vertexData[2].size());


    // Face indices for 2 triangles
    std::vector<uint32_t> faces = {
        0, 2, 1,
        0, 3, 2
    };

    return std::make_shared<Mesh>(4, 2, attributeNames, attributeTypes, vertexData, faces);
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::icosphere(
    int subdivisionLevel,
    const glm::vec4& color,
    bool onlyPosition
)
{
    if (subdivisionLevel < 0) subdivisionLevel = 0;

    // Base icosahedron (unit radius after normalization)
    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::vector<glm::vec3> vertices = {
        {-1.0f,  t,  0.0f},
        { 1.0f,  t,  0.0f},
        {-1.0f, -t,  0.0f},
        { 1.0f, -t,  0.0f},

        { 0.0f, -1.0f,  t},
        { 0.0f,  1.0f,  t},
        { 0.0f, -1.0f, -t},
        { 0.0f,  1.0f, -t},

        { t,  0.0f, -1.0f},
        { t,  0.0f,  1.0f},
        {-t,  0.0f, -1.0f},
        {-t,  0.0f,  1.0f},
    };
    for (auto& v : vertices) v = glm::normalize(v);

    std::vector<uint32_t> indices = {
        0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10, 0, 10, 11,
        1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,   3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,   2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1
    };

    // Midpoint cache to avoid duplicate vertices across shared edges.
    struct EdgeKey
    {
        uint32_t a;
        uint32_t b;
        bool operator==(const EdgeKey& other) const { return a == other.a && b == other.b; }
    };
    struct EdgeKeyHash
    {
        size_t operator()(const EdgeKey& k) const
        {
            // 64-bit mix of 2x32-bit
            return (static_cast<size_t>(k.a) << 32) ^ static_cast<size_t>(k.b);
        }
    };

    auto midpointIndex = [&](uint32_t i0, uint32_t i1, std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& cache) -> uint32_t
    {
        uint32_t a = std::min(i0, i1);
        uint32_t b = std::max(i0, i1);
        EdgeKey key{a, b};
        if (auto it = cache.find(key); it != cache.end()) return it->second;

        glm::vec3 mid = glm::normalize((vertices[a] + vertices[b]) * 0.5f);
        uint32_t idx = static_cast<uint32_t>(vertices.size());
        vertices.push_back(mid);
        cache.emplace(key, idx);
        return idx;
    };

    for (int s = 0; s < subdivisionLevel; ++s)
    {
        std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> cache;
        std::vector<uint32_t> newIndices;
        newIndices.reserve(indices.size() * 4);

        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i + 0];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];

            uint32_t a = midpointIndex(i0, i1, cache);
            uint32_t b = midpointIndex(i1, i2, cache);
            uint32_t c = midpointIndex(i2, i0, cache);

            // 4 new triangles
            newIndices.insert(newIndices.end(), {i0, a, c});
            newIndices.insert(newIndices.end(), {i1, b, a});
            newIndices.insert(newIndices.end(), {i2, c, b});
            newIndices.insert(newIndices.end(), {a, b, c});
        }
        indices.swap(newIndices);
    }

    // Ensure winding is consistently outward (robust across subdivision parity).
    // We test the first triangle on the unit sphere topology and, if needed, flip all triangles.
    if (indices.size() >= 3)
    {
        const auto i0 = indices[0];
        const auto i1 = indices[1];
        const auto i2 = indices[2];
        const glm::vec3 p0 = vertices[i0];
        const glm::vec3 p1 = vertices[i1];
        const glm::vec3 p2 = vertices[i2];
        const glm::vec3 n = glm::cross(p1 - p0, p2 - p0);
        const glm::vec3 c = (p0 + p1 + p2) / 3.0f;
        if (glm::dot(n, c) < 0.0f)
        {
            for (size_t k = 0; k + 2 < indices.size(); k += 3)
            {
                std::swap(indices[k + 1], indices[k + 2]);
            }
        }
    }

    // Attributes
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    if (!onlyPosition)
    {
        colors.assign(vertices.size(), color);
        normals.resize(vertices.size());
        uvs.resize(vertices.size());

        constexpr float PI = 3.14159265358979323846f;
        for (size_t i = 0; i < vertices.size(); ++i)
        {
            const glm::vec3 n = glm::normalize(vertices[i]);
            normals[i] = n;

            // Spherical UV mapping
            float u = std::atan2(n.z, n.x) / (2.0f * PI) + 0.5f;
            float v = std::asin(glm::clamp(n.y, -1.0f, 1.0f)) / PI + 0.5f;
            uvs[i] = {u, v};
        }
    }

    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };

        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    if (!onlyPosition)
    {
        mesh->recalculateNormals();
    }

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetPD(
    const Planet& planet,
    float minDistance,
    const glm::vec4& color,
    bool onlyPosition,
    int maxAttempts
)
{
    constexpr float PI = 3.14159265358979323846f;

    // Poisson Disk Sampling in UV space
    std::vector<glm::vec2> samples;
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_real_distribution<float> dis(0.0f, 1.0f);

    // Grid for spatial lookup (acceleration structure)
    const float cellSize = minDistance / std::sqrt(2.0f);
    const int gridWidth = static_cast<int>(std::ceil(1.0f / cellSize));
    const int gridHeight = static_cast<int>(std::ceil(1.0f / cellSize));
    std::vector<std::vector<int>> grid(gridWidth * gridHeight);

    auto gridIndex = [&](const glm::vec2& p) -> int {
        int x = static_cast<int>(p.x / cellSize);
        int y = static_cast<int>(p.y / cellSize);
        x = glm::clamp(x, 0, gridWidth - 1);
        y = glm::clamp(y, 0, gridHeight - 1);
        return y * gridWidth + x;
    };

    // Start with random seed point
    glm::vec2 firstSample(dis(gen), dis(gen));
    samples.push_back(firstSample);
    grid[gridIndex(firstSample)].push_back(0);

    std::vector<int> activeList = {0};

    // Poisson disk sampling algorithm
    while (!activeList.empty())
    {
        int randomIndex = std::uniform_int_distribution<int>(0, activeList.size() - 1)(gen);
        int sampleIndex = activeList[randomIndex];
        const glm::vec2& sample = samples[sampleIndex];

        bool found = false;
        for (int attempt = 0; attempt < maxAttempts; ++attempt)
        {
            float angle = dis(gen) * 2.0f * PI;
            float radius = minDistance * (1.0f + dis(gen));
            glm::vec2 candidate = sample + glm::vec2(std::cos(angle), std::sin(angle)) * radius;

            // Wrap around for u coordinate (periodic boundary)
            candidate.x = candidate.x - std::floor(candidate.x);

            // Check if candidate is valid
            if (candidate.y < 0.0f || candidate.y > 1.0f) continue;

            // Check distance to nearby samples
            bool valid = true;
            int gx = static_cast<int>(candidate.x / cellSize);
            int gy = static_cast<int>(candidate.y / cellSize);

            for (int dy = -2; dy <= 2; ++dy)
            {
                for (int dx = -2; dx <= 2; ++dx)
                {
                    int nx = gx + dx;
                    int ny = gy + dy;

                    // Handle periodic boundary in x
                    if (nx < 0) nx += gridWidth;
                    if (nx >= gridWidth) nx -= gridWidth;

                    if (ny < 0 || ny >= gridHeight) continue;

                    int idx = ny * gridWidth + nx;
                    for (int existingIdx : grid[idx])
                    {
                        glm::vec2 diff = candidate - samples[existingIdx];
                        // Handle wrapping for u coordinate
                        if (diff.x > 0.5f) diff.x -= 1.0f;
                        if (diff.x < -0.5f) diff.x += 1.0f;

                        if (glm::length(diff) < minDistance)
                        {
                            valid = false;
                            break;
                        }
                    }
                    if (!valid) break;
                }
                if (!valid) break;
            }

            if (valid)
            {
                int newIdx = static_cast<int>(samples.size());
                samples.push_back(candidate);
                grid[gridIndex(candidate)].push_back(newIdx);
                activeList.push_back(newIdx);
                found = true;
                break;
            }
        }

        if (!found)
        {
            activeList.erase(activeList.begin() + randomIndex);
        }
    }

    // Convert UV samples to 3D positions on planet surface
    std::vector<glm::vec3> vertices(samples.size());
    std::vector<glm::vec3> normals;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec2> uvs;

    if (!onlyPosition)
    {
        normals.resize(samples.size());
        colors.assign(samples.size(), color);
        uvs = samples;
    }

    for (size_t i = 0; i < samples.size(); ++i)
    {
        const glm::vec2& uv = samples[i];
        vertices[i] = planet.evaluate(uv.x, uv.y);
        if (!onlyPosition)
        {
            normals[i] = glm::normalize(planet.normal(uv.x, uv.y));
        }
    }

    // Create Delaunay triangulation (simple approach using existing triangulation)
    // For now, use a simple approach: create triangles from neighboring points
    // A proper Delaunay would be better but requires external library

    // Simple triangulation: use grid-based connectivity
    std::vector<uint32_t> indices;

    // Build spatial hash for vertex lookup
    std::unordered_map<int, std::vector<int>> spatialHash;
    for (size_t i = 0; i < samples.size(); ++i)
    {
        int hash = gridIndex(samples[i]);
        spatialHash[hash].push_back(static_cast<int>(i));
    }

    // For each vertex, find nearby vertices and create triangles
    for (size_t i = 0; i < samples.size(); ++i)
    {
        const glm::vec2& uv = samples[i];
        int gx = static_cast<int>(uv.x / cellSize);
        int gy = static_cast<int>(uv.y / cellSize);

        std::vector<int> neighbors;
        for (int dy = -1; dy <= 1; ++dy)
        {
            for (int dx = -1; dx <= 1; ++dx)
            {
                int nx = gx + dx;
                int ny = gy + dy;

                if (nx < 0) nx += gridWidth;
                if (nx >= gridWidth) nx -= gridWidth;
                if (ny < 0 || ny >= gridHeight) continue;

                int hash = ny * gridWidth + nx;
                if (spatialHash.find(hash) != spatialHash.end())
                {
                    for (int idx : spatialHash[hash])
                    {
                        if (idx != static_cast<int>(i))
                        {
                            neighbors.push_back(idx);
                        }
                    }
                }
            }
        }

        // Sort neighbors by angle around current vertex
        std::sort(neighbors.begin(), neighbors.end(), [&](int a, int b) {
            glm::vec2 da = samples[a] - uv;
            glm::vec2 db = samples[b] - uv;
            // Handle wrapping
            if (da.x > 0.5f) da.x -= 1.0f;
            if (da.x < -0.5f) da.x += 1.0f;
            if (db.x > 0.5f) db.x -= 1.0f;
            if (db.x < -0.5f) db.x += 1.0f;
            return std::atan2(da.y, da.x) < std::atan2(db.y, db.x);
        });

        // Create triangles with adjacent neighbors
        for (size_t j = 0; j < neighbors.size(); ++j)
        {
            size_t next = (j + 1) % neighbors.size();
            int n1 = neighbors[j];
            int n2 = neighbors[next];

            // Check if this triangle hasn't been added yet
            // Simple check: only add if i < n1 < n2 (arbitrary ordering to avoid duplicates)
            if (static_cast<int>(i) < n1 && n1 < n2)
            {
                indices.push_back(static_cast<uint32_t>(i));
                indices.push_back(static_cast<uint32_t>(n1));
                indices.push_back(static_cast<uint32_t>(n2));
            }
        }
    }

    // Pack mesh data
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromIcoPlanet(
    const Planet& planet,
    int subdivisionLevel,
    const glm::vec4& color,
    bool onlyPosition
)
{
    if (subdivisionLevel < 0) subdivisionLevel = 0;

    // Build an icosphere topology we can remap onto the planet surface.
    // We intentionally rebuild the vertices/indices here to keep indices in sync.
    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::vector<glm::vec3> baseVertices = {
        {-1.0f,  t,  0.0f},
        { 1.0f,  t,  0.0f},
        {-1.0f, -t,  0.0f},
        { 1.0f, -t,  0.0f},

        { 0.0f, -1.0f,  t},
        { 0.0f,  1.0f,  t},
        { 0.0f, -1.0f, -t},
        { 0.0f,  1.0f, -t},

        { t,  0.0f, -1.0f},
        { t,  0.0f,  1.0f},
        {-t,  0.0f, -1.0f},
        {-t,  0.0f,  1.0f},
    };
    for (auto& v : baseVertices) v = glm::normalize(v);

    // Base icosahedron faces (standard winding). We'll enforce outward CCW after subdivision.
    std::vector<uint32_t> indices = {
        0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10, 0, 10, 11,
        1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,   3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,   2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1
    };

    struct EdgeKey { uint32_t a; uint32_t b; bool operator==(const EdgeKey& o) const { return a==o.a && b==o.b; } };
    struct EdgeKeyHash { size_t operator()(const EdgeKey& k) const { return (static_cast<size_t>(k.a) << 32) ^ static_cast<size_t>(k.b); } };
    auto midpointIndex = [&](uint32_t i0, uint32_t i1, std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& cache) -> uint32_t
    {
        uint32_t a = std::min(i0, i1);
        uint32_t b = std::max(i0, i1);
        EdgeKey key{a, b};
        if (auto it = cache.find(key); it != cache.end()) return it->second;
        glm::vec3 mid = glm::normalize((baseVertices[a] + baseVertices[b]) * 0.5f);
        uint32_t idx = static_cast<uint32_t>(baseVertices.size());
        baseVertices.push_back(mid);
        cache.emplace(key, idx);
        return idx;
    };

    for (int s = 0; s < subdivisionLevel; ++s)
    {
        std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> cache;
        std::vector<uint32_t> newIndices;
        newIndices.reserve(indices.size() * 4);
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i + 0];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            uint32_t a = midpointIndex(i0, i1, cache);
            uint32_t b = midpointIndex(i1, i2, cache);
            uint32_t c = midpointIndex(i2, i0, cache);
            // Standard subdivision that preserves the parent's winding.
            newIndices.insert(newIndices.end(), {i0, a, c});
            newIndices.insert(newIndices.end(), {i1, b, a});
            newIndices.insert(newIndices.end(), {i2, c, b});
            newIndices.insert(newIndices.end(), {a, b, c});
        }
        indices.swap(newIndices);
    }

    // Remap vertices onto planet surface.
    constexpr float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> vertices;
    vertices.resize(baseVertices.size());

    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    if (!onlyPosition)
    {
        colors.assign(baseVertices.size(), color);
        normals.resize(baseVertices.size());
        uvs.resize(baseVertices.size());
    }

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        const glm::vec3 dir = glm::normalize(baseVertices[i]);

        // Convert direction to (u,v) in [0,1]x[0,1]
        float u = std::atan2(dir.z, dir.x) / (2.0f * PI) + 0.5f;
        float v = std::asin(glm::clamp(dir.y, -1.0f, 1.0f)) / PI + 0.5f;

        vertices[i] = planet.evaluate(u, v);
        if (!onlyPosition)
        {
            normals[i] = glm::normalize(planet.normal(u, v));
            uvs[i] = {u, v};
        }
    }

    // Enforce CCW winding on the *remapped* planet geometry.
    // We compare the geometric normal (from winding) with the surface normal (from planet).
    // If they point in opposite directions, the winding is wrong and needs to be flipped.
    if (!onlyPosition)
    {
        for (size_t k = 0; k + 2 < indices.size(); k += 3)
        {
            const uint32_t i0 = indices[k + 0];
            const uint32_t i1 = indices[k + 1];
            const uint32_t i2 = indices[k + 2];

            const glm::vec3& p0 = vertices[i0];
            const glm::vec3& p1 = vertices[i1];
            const glm::vec3& p2 = vertices[i2];

            // Geometric normal from current winding order
            const glm::vec3 geometricNormal = glm::cross(p1 - p0, p2 - p0);
            if (glm::length2(geometricNormal) <= 1e-20f)
            {
                continue;
            }

            // Expected normal from surface (average of vertex normals)
            const glm::vec3 surfaceNormal = normals[i0] + normals[i1] + normals[i2];

            // If they point in opposite directions, flip the triangle
            if (glm::dot(geometricNormal, surfaceNormal) < 0.0f)
            {
                std::swap(indices[k + 1], indices[k + 2]);
            }
        }
    }

    // Pack mesh data
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    // TEST: Don't recalculate normals - use the ones from planet.normal()
    // if (!onlyPosition)
    // {
    //     mesh->recalculateNormals();
    // }

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromIcoPlanetRockyfied(
    const Planet& planet,
    int subdivisionLevel,
    const glm::vec4& color,
    bool onlyPosition,
    int fractalOctaves,
    float fractalIntensity,
    float fractalScale
)
{
    if (subdivisionLevel < 0) subdivisionLevel = 0;

    // --- Build icosphere topology (same as fromIcoPlanet) ---
    const float t = (1.0f + std::sqrt(5.0f)) * 0.5f;
    std::vector<glm::vec3> baseVertices = {
        {-1.0f,  t,  0.0f},
        { 1.0f,  t,  0.0f},
        {-1.0f, -t,  0.0f},
        { 1.0f, -t,  0.0f},

        { 0.0f, -1.0f,  t},
        { 0.0f,  1.0f,  t},
        { 0.0f, -1.0f, -t},
        { 0.0f,  1.0f, -t},

        { t,  0.0f, -1.0f},
        { t,  0.0f,  1.0f},
        {-t,  0.0f, -1.0f},
        {-t,  0.0f,  1.0f},
    };
    for (auto& v : baseVertices) v = glm::normalize(v);

    std::vector<uint32_t> indices = {
        0, 11, 5,  0, 5, 1,  0, 1, 7,  0, 7, 10, 0, 10, 11,
        1, 5, 9,   5, 11, 4,  11, 10, 2, 10, 7, 6,  7, 1, 8,
        3, 9, 4,   3, 4, 2,   3, 2, 6,   3, 6, 8,   3, 8, 9,
        4, 9, 5,   2, 4, 11,  6, 2, 10,  8, 6, 7,   9, 8, 1
    };

    struct EdgeKey { uint32_t a; uint32_t b; bool operator==(const EdgeKey& o) const { return a==o.a && b==o.b; } };
    struct EdgeKeyHash { size_t operator()(const EdgeKey& k) const { return (static_cast<size_t>(k.a) << 32) ^ static_cast<size_t>(k.b); } };
    auto midpointIndex = [&](uint32_t i0, uint32_t i1, std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash>& cache) -> uint32_t
    {
        uint32_t a = std::min(i0, i1);
        uint32_t b = std::max(i0, i1);
        EdgeKey key{a, b};
        if (auto it = cache.find(key); it != cache.end()) return it->second;
        glm::vec3 mid = glm::normalize((baseVertices[a] + baseVertices[b]) * 0.5f);
        uint32_t idx = static_cast<uint32_t>(baseVertices.size());
        baseVertices.push_back(mid);
        cache.emplace(key, idx);
        return idx;
    };

    for (int s = 0; s < subdivisionLevel; ++s)
    {
        std::unordered_map<EdgeKey, uint32_t, EdgeKeyHash> cache;
        std::vector<uint32_t> newIndices;
        newIndices.reserve(indices.size() * 4);
        for (size_t i = 0; i < indices.size(); i += 3)
        {
            uint32_t i0 = indices[i + 0];
            uint32_t i1 = indices[i + 1];
            uint32_t i2 = indices[i + 2];
            uint32_t a = midpointIndex(i0, i1, cache);
            uint32_t b = midpointIndex(i1, i2, cache);
            uint32_t c = midpointIndex(i2, i0, cache);
            newIndices.insert(newIndices.end(), {i0, a, c});
            newIndices.insert(newIndices.end(), {i1, b, a});
            newIndices.insert(newIndices.end(), {i2, c, b});
            newIndices.insert(newIndices.end(), {a, b, c});
        }
        indices.swap(newIndices);
    }

    // --- Remap onto planet + rocky displacement ---
    constexpr float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> vertices(baseVertices.size());
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec4> colors;
    if (!onlyPosition)
    {
        uvs.resize(baseVertices.size());
        colors.assign(baseVertices.size(), color);
    }

    // Store reference normals before displacement for winding check
    std::vector<glm::vec3> referenceNormals;
    if (!onlyPosition)
    {
        referenceNormals.resize(baseVertices.size());
    }

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        const glm::vec3 dir = glm::normalize(baseVertices[i]);
        float u = std::atan2(dir.z, dir.x) / (2.0f * PI) + 0.5f;
        float v = std::asin(glm::clamp(dir.y, -1.0f, 1.0f)) / PI + 0.5f;

        auto normal = planet.normal(u, v);
        auto position = planet.evaluate(u, v);

        // Use noise for rocky detail
        float displacement = Geometry::Mesh::ridgedFBM(position * fractalScale, fractalOctaves);
        position += normal * displacement * fractalIntensity;
        vertices[i] = position;

        if (!onlyPosition)
        {
            referenceNormals[i] = glm::normalize(normal);
            uvs[i] = {u, v};
        }
    }

    // Enforce CCW winding on the remapped geometry.
    // We compare the geometric normal (from winding) with the reference surface normal.
    if (!onlyPosition)
    {
        for (size_t k = 0; k + 2 < indices.size(); k += 3)
        {
            const uint32_t i0 = indices[k + 0];
            const uint32_t i1 = indices[k + 1];
            const uint32_t i2 = indices[k + 2];
            const glm::vec3& p0 = vertices[i0];
            const glm::vec3& p1 = vertices[i1];
            const glm::vec3& p2 = vertices[i2];

            // Geometric normal from current winding order
            const glm::vec3 geometricNormal = glm::cross(p1 - p0, p2 - p0);
            if (glm::length2(geometricNormal) <= 1e-20f)
            {
                continue;
            }

            // Expected normal from surface (average of reference normals before displacement)
            const glm::vec3 surfaceNormal = referenceNormals[i0] + referenceNormals[i1] + referenceNormals[i2];

            // If they point in opposite directions, flip the triangle
            if (glm::dot(geometricNormal, surfaceNormal) < 0.0f)
            {
                std::swap(indices[k + 1], indices[k + 2]);
            }
        }
    }

    // Normals will be recalculated from final geometry (after winding fix).
    std::vector<glm::vec3> normals;
    if (!onlyPosition)
    {
        normals.assign(vertices.size(), glm::vec3(0.0f));
    }

    // Pack mesh data
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };

        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    if (!onlyPosition)
    {
        mesh->recalculateNormals();
    }

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromCubePlanet(
    const Planet& planet,
    int subdivisionLevel,
    const glm::vec4& color,
    bool onlyPosition
)
{
    if (subdivisionLevel < 0) subdivisionLevel = 0;

    // Build a cubesphere topology
    // Start with 6 faces of a cube, each subdivided into a grid
    const int gridSize = (1 << subdivisionLevel) + 1; // 2^subdivisionLevel + 1
    const float step = 2.0f / (gridSize - 1);

    std::vector<glm::vec3> baseVertices;
    std::vector<uint32_t> indices;

    // Lambda to add a cube face
    auto addCubeFace = [&](int axis, int sign) {
        // axis: 0=X, 1=Y, 2=Z; sign: -1 or +1
        int startIdx = static_cast<int>(baseVertices.size());

        // Generate grid for this face
        for (int i = 0; i < gridSize; ++i)
        {
            for (int j = 0; j < gridSize; ++j)
            {
                float u = -1.0f + i * step;
                float v = -1.0f + j * step;

                glm::vec3 p;
                if (axis == 0) { // X face
                    p = glm::vec3(sign * 1.0f, u, v);
                } else if (axis == 1) { // Y face
                    p = glm::vec3(u, sign * 1.0f, v);
                } else { // Z face
                    p = glm::vec3(u, v, sign * 1.0f);
                }

                // Project onto sphere
                p = glm::normalize(p);
                baseVertices.push_back(p);
            }
        }

        // Generate indices for this face
        for (int i = 0; i < gridSize - 1; ++i)
        {
            for (int j = 0; j < gridSize - 1; ++j)
            {
                int idx0 = startIdx + i * gridSize + j;
                int idx1 = startIdx + i * gridSize + (j + 1);
                int idx2 = startIdx + (i + 1) * gridSize + (j + 1);
                int idx3 = startIdx + (i + 1) * gridSize + j;

                // Two triangles per quad
                if (sign > 0) {
                    indices.push_back(idx0);
                    indices.push_back(idx1);
                    indices.push_back(idx2);

                    indices.push_back(idx0);
                    indices.push_back(idx2);
                    indices.push_back(idx3);
                } else {
                    // Flip winding for negative faces
                    indices.push_back(idx0);
                    indices.push_back(idx2);
                    indices.push_back(idx1);

                    indices.push_back(idx0);
                    indices.push_back(idx3);
                    indices.push_back(idx2);
                }
            }
        }
    };

    // Add all 6 cube faces
    addCubeFace(0, 1);  // +X
    addCubeFace(0, -1); // -X
    addCubeFace(1, 1);  // +Y
    addCubeFace(1, -1); // -Y
    addCubeFace(2, 1);  // +Z
    addCubeFace(2, -1); // -Z

    // Remap vertices onto planet surface
    constexpr float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> vertices;
    vertices.resize(baseVertices.size());

    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> uvs;
    if (!onlyPosition)
    {
        colors.assign(baseVertices.size(), color);
        normals.resize(baseVertices.size());
        uvs.resize(baseVertices.size());
    }

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        const glm::vec3 dir = glm::normalize(baseVertices[i]);

        // Convert direction to (u,v) in [0,1]x[0,1]
        float u = std::atan2(dir.z, dir.x) / (2.0f * PI) + 0.5f;
        float v = std::asin(glm::clamp(dir.y, -1.0f, 1.0f)) / PI + 0.5f;

        vertices[i] = planet.evaluate(u, v);
        if (!onlyPosition)
        {
            normals[i] = glm::normalize(planet.normal(u, v));
            uvs[i] = {u, v};
        }
    }

    // Enforce CCW winding on the remapped geometry
    if (!onlyPosition)
    {
        for (size_t k = 0; k + 2 < indices.size(); k += 3)
        {
            const uint32_t i0 = indices[k + 0];
            const uint32_t i1 = indices[k + 1];
            const uint32_t i2 = indices[k + 2];

            const glm::vec3& p0 = vertices[i0];
            const glm::vec3& p1 = vertices[i1];
            const glm::vec3& p2 = vertices[i2];

            // Geometric normal from current winding order
            const glm::vec3 geometricNormal = glm::cross(p1 - p0, p2 - p0);
            if (glm::length2(geometricNormal) <= 1e-20f)
            {
                continue;
            }

            // Expected normal from surface (average of vertex normals)
            const glm::vec3 surfaceNormal = normals[i0] + normals[i1] + normals[i2];

            // If they point in opposite directions, flip the triangle
            if (glm::dot(geometricNormal, surfaceNormal) < 0.0f)
            {
                std::swap(indices[k + 1], indices[k + 2]);
            }
        }
    }

    // Pack mesh data
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromCubePlanetRockyfied(
    const Planet& planet,
    int subdivisionLevel,
    const glm::vec4& color,
    bool onlyPosition,
    int fractalOctaves,
    float fractalIntensity,
    float fractalScale
)
{
    if (subdivisionLevel < 0) subdivisionLevel = 0;

    // Build a cubesphere topology (same as fromCubePlanet)
    const int gridSize = (1 << subdivisionLevel) + 1;
    const float step = 2.0f / (gridSize - 1);

    std::vector<glm::vec3> baseVertices;
    std::vector<uint32_t> indices;

    auto addCubeFace = [&](int axis, int sign) {
        int startIdx = static_cast<int>(baseVertices.size());

        for (int i = 0; i < gridSize; ++i)
        {
            for (int j = 0; j < gridSize; ++j)
            {
                float u = -1.0f + i * step;
                float v = -1.0f + j * step;

                glm::vec3 p;
                if (axis == 0) {
                    p = glm::vec3(sign * 1.0f, u, v);
                } else if (axis == 1) {
                    p = glm::vec3(u, sign * 1.0f, v);
                } else {
                    p = glm::vec3(u, v, sign * 1.0f);
                }

                p = glm::normalize(p);
                baseVertices.push_back(p);
            }
        }

        for (int i = 0; i < gridSize - 1; ++i)
        {
            for (int j = 0; j < gridSize - 1; ++j)
            {
                int idx0 = startIdx + i * gridSize + j;
                int idx1 = startIdx + i * gridSize + (j + 1);
                int idx2 = startIdx + (i + 1) * gridSize + (j + 1);
                int idx3 = startIdx + (i + 1) * gridSize + j;

                if (sign > 0) {
                    indices.push_back(idx0);
                    indices.push_back(idx1);
                    indices.push_back(idx2);

                    indices.push_back(idx0);
                    indices.push_back(idx2);
                    indices.push_back(idx3);
                } else {
                    indices.push_back(idx0);
                    indices.push_back(idx2);
                    indices.push_back(idx1);

                    indices.push_back(idx0);
                    indices.push_back(idx3);
                    indices.push_back(idx2);
                }
            }
        }
    };

    addCubeFace(0, 1);
    addCubeFace(0, -1);
    addCubeFace(1, 1);
    addCubeFace(1, -1);
    addCubeFace(2, 1);
    addCubeFace(2, -1);

    // Remap onto planet + rocky displacement
    constexpr float PI = 3.14159265358979323846f;
    std::vector<glm::vec3> vertices(baseVertices.size());
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec4> colors;
    if (!onlyPosition)
    {
        uvs.resize(baseVertices.size());
        colors.assign(baseVertices.size(), color);
    }

    // Store reference normals before displacement for winding check
    std::vector<glm::vec3> referenceNormals;
    if (!onlyPosition)
    {
        referenceNormals.resize(baseVertices.size());
    }

    for (size_t i = 0; i < baseVertices.size(); ++i)
    {
        const glm::vec3 dir = glm::normalize(baseVertices[i]);
        float u = std::atan2(dir.z, dir.x) / (2.0f * PI) + 0.5f;
        float v = std::asin(glm::clamp(dir.y, -1.0f, 1.0f)) / PI + 0.5f;

        auto normal = planet.normal(u, v);
        auto position = planet.evaluate(u, v);

        // Use noise for rocky detail
        float displacement = Geometry::Mesh::ridgedFBM(position * fractalScale, fractalOctaves);
        position += normal * displacement * fractalIntensity;
        vertices[i] = position;

        if (!onlyPosition)
        {
            referenceNormals[i] = glm::normalize(normal);
            uvs[i] = {u, v};
        }
    }

    // Enforce CCW winding on the remapped geometry
    if (!onlyPosition)
    {
        for (size_t k = 0; k + 2 < indices.size(); k += 3)
        {
            const uint32_t i0 = indices[k + 0];
            const uint32_t i1 = indices[k + 1];
            const uint32_t i2 = indices[k + 2];
            const glm::vec3& p0 = vertices[i0];
            const glm::vec3& p1 = vertices[i1];
            const glm::vec3& p2 = vertices[i2];

            const glm::vec3 geometricNormal = glm::cross(p1 - p0, p2 - p0);
            if (glm::length2(geometricNormal) <= 1e-20f)
            {
                continue;
            }

            const glm::vec3 surfaceNormal = referenceNormals[i0] + referenceNormals[i1] + referenceNormals[i2];

            if (glm::dot(geometricNormal, surfaceNormal) < 0.0f)
            {
                std::swap(indices[k + 1], indices[k + 2]);
            }
        }
    }

    // Normals will be recalculated from final geometry
    std::vector<glm::vec3> normals;
    if (!onlyPosition)
    {
        normals.assign(vertices.size(), glm::vec3(0.0f));
    }

    // Pack mesh data
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    std::vector<std::vector<uint8_t>> vertexData;

    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };

        vertexData.resize(4);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(uvs.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), uvs.data(), vertexData[3].size());
    }
    else
    {
        attributeNames = {Core::VertexAttributeName::Position};
        attributeTypes = {Core::VertexAttributeType::Float3};
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    auto mesh = std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );

    if (!onlyPosition)
    {
        mesh->recalculateNormals();
    }

    return mesh;
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromBSpline(
        const BSpline& curve,
        float step,
        const glm::vec4& color
    )
{
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4
    };

    auto positions = std::vector<glm::vec3>();
    auto t = 0.0f;
    while (t < 1.0f)
    {
        positions.push_back(curve.evaluate(t));
        t += step; // increment by 0.01
    }
    auto lines = std::vector<glm::vec3>();
    for (int i = 0; i < positions.size() - 1; ++i)
    {
        lines.push_back(positions[i]);
        lines.push_back(positions[i + 1]);
    }
    auto colors = std::vector<glm::vec4>(lines.size());
    for (int i = 0; i < colors.size(); ++i)
    {
        colors[i] = color; // purple color
    }
    std::vector<std::vector<uint8_t>> vertexData(2);
    vertexData[0].resize(lines.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), lines.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    return std::make_shared<Mesh>(
        static_cast<int>(lines.size()),
        0,
        attributeNames,
        attributeTypes,
        vertexData,
        std::vector<uint32_t>() // no faces for lines
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPolygon(
    const std::vector<glm::vec3>& positions,
    const glm::vec4& color,
    bool addInnerVertices
    )
{
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4
    };

    auto lines = std::vector<glm::vec3>();
    for (int i = 0; i < positions.size(); ++i)
    {
        lines.push_back(positions[i]);
        if (addInnerVertices and i + 1 < positions.size()) lines.push_back(positions[i + 1]);
    }
    auto colors = std::vector<glm::vec4>(lines.size());
    for (int i = 0; i < colors.size(); ++i)
    {
        colors[i] = color; // purple color
    }
    std::vector<std::vector<uint8_t>> vertexData(2);
    vertexData[0].resize(lines.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), lines.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    return std::make_shared<Mesh>(
        static_cast<int>(lines.size()),
        0,
        attributeNames,
        attributeTypes,
        vertexData,
        std::vector<uint32_t>() // no faces for lines
    );
}


std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanet(
    const Planet& planet,
    const glm::vec4& color,
    float samplingRes,
    bool onlyPosition
)
{
    // nU e nV: numero di sample per direzione
    int nU = static_cast<int>(1.0f / samplingRes) + 1;
    int nV = static_cast<int>(1.0f / samplingRes) + 1;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textCoords;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);

    if (!onlyPosition)
    {
        textCoords.emplace_back(0.0f, 0.0f);
        colors.push_back(color);
        //normals.emplace_back(glm::normalize(planet.normal(0.0f, 0.0f)));
        normals.emplace_back(glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1; i < nV - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));
            if (!onlyPosition)
            {
                textCoords.emplace_back(u, v);
                colors.push_back(color);
                normals.emplace_back(glm::normalize(planet.normal(u, v)));
            }
        }
    }

    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);
    if (!onlyPosition)
    {
        textCoords.emplace_back(0.0f, 1.0f);
        colors.push_back(color);
        //normals.emplace_back(glm::normalize(planet.normal(0.0f, 1.0f)));
        normals.emplace_back(glm::vec3(0.0f, -1.0f, 0.0f));
    }

    // South POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j)
    {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(v1);
        indices.push_back(southPoleIdx);
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v0);

            // Second Triangle
            indices.push_back(v3);
            indices.push_back(v2);
            indices.push_back(v1);
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    auto northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(northPoleIdx);
        indices.push_back(v1);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames;
    if (!onlyPosition) {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
    } else {
        attributeNames = {
            Core::VertexAttributeName::Position
        };
    }

    std::vector<Core::VertexAttributeType> attributeTypes;
    if (!onlyPosition) {
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
    } else {
        attributeTypes = {
            Core::VertexAttributeType::Float3
        };
    }

    std::vector<std::vector<uint8_t>> vertexData; // Changed from 3 to 4
    if (!onlyPosition) {
        vertexData.resize(4); // Changed from 3 to 4
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(textCoords.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), textCoords.data(), vertexData[3].size());
    } else {
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetRockyfied(
        const Planet& planet,
        const glm::vec4& color,
        float samplingRes,
        bool onlyPosition,
        int fractalOctaves,
        float fractalIntensity
        )
{
    // nU e nV: numero di sample per direzione
    int nU = static_cast<int>(1.0f / samplingRes) + 1;
    int nV = static_cast<int>(1.0f / samplingRes) + 1;
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textCoords;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    auto southPoleNormal = glm::vec3(0.0f, 1.0f, 0.0f);
    southPole += southPoleNormal * Geometry::Mesh::ridgedFBM(southPole, fractalOctaves) * fractalIntensity;
    vertices.push_back(southPole);

    if (!onlyPosition)
    {
        textCoords.emplace_back(0.0f, 0.0f);
        colors.push_back(color);
        //normals.emplace_back(glm::normalize(planet.normal(0.0f, 0.0f)));
        normals.emplace_back(southPoleNormal);
    }

    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1; i < nV - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            auto normal = planet.normal(u, v);
            auto position = planet.evaluate(u, v);
            position += normal * Geometry::Mesh::ridgedFBM(position, fractalOctaves) * fractalIntensity; // Adjust the scale factor as needed
            vertices.push_back(position);
            if (!onlyPosition)
            {
                textCoords.emplace_back(u, v);
                colors.push_back(color);
                //normals.emplace_back(glm::normalize(normal));
                normals.emplace_back(0.0f);
            }
        }
    }

    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    auto northPoleNormal = glm::vec3(0.0f, -1.0f, 0.0f);
    northPole += northPoleNormal * Geometry::Mesh::ridgedFBM(northPole, fractalOctaves) * fractalIntensity;
    vertices.push_back(northPole);
    if (!onlyPosition)
    {
        textCoords.emplace_back(0.0f, 1.0f);
        colors.push_back(color);
        //normals.emplace_back(glm::normalize(planet.normal(0.0f, 1.0f)));
        normals.emplace_back(northPoleNormal);
    }

    // South POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j)
    {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(v1);
        indices.push_back(southPoleIdx);
        glm::vec3 n = glm::cross(vertices[v2] - vertices[southPoleIdx], vertices[v1] - vertices[southPoleIdx]);
        normals[southPoleIdx] += n;
        normals[v1] += n;
        normals[v2] += n;
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v0);

            // add normals
            glm::vec3 n1 = glm::cross(vertices[v1] - vertices[v0], vertices[v2] - vertices[v0]);
            normals[v0] += n1;
            normals[v1] += n1;
            normals[v2] += n1;

            // Second Triangle
            indices.push_back(v3);
            indices.push_back(v2);
            indices.push_back(v1);

            // add normals
            glm::vec3 n2 = glm::cross(vertices[v3] - vertices[v1], vertices[v2] - vertices[v1]);
            normals[v1] += n2;
            normals[v2] += n2;
            normals[v3] += n2;
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    auto northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(northPoleIdx);
        indices.push_back(v1);
        glm::vec3 n = glm::cross(vertices[v2] - vertices[v1], vertices[northPoleIdx] - vertices[v1]);
        normals[northPoleIdx] += n;
        normals[v1] += n;
        normals[v2] += n;
    }

    for (auto & normal : normals)
    {
        normal = glm::normalize(normal);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames;
    if (!onlyPosition) {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
    } else {
        attributeNames = {
            Core::VertexAttributeName::Position
        };
    }

    std::vector<Core::VertexAttributeType> attributeTypes;
    if (!onlyPosition) {
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
    } else {
        attributeTypes = {
            Core::VertexAttributeType::Float3
        };
    }

    std::vector<std::vector<uint8_t>> vertexData; // Changed from 3 to 4
    if (!onlyPosition) {
        vertexData.resize(4); // Changed from 3 to 4
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
        vertexData[1].resize(colors.size() * sizeof(glm::vec4));
        std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(textCoords.size() * sizeof(glm::vec2));
        std::memcpy(vertexData[3].data(), textCoords.data(), vertexData[3].size());
    } else {
        vertexData.resize(1);
        vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
        std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    }
    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

// GPU-accelerated mesh builder using Metal (metal-cpp)
std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetGPU(
    const Planet& planet,
    const glm::vec4& color,
    float samplingRes,
    bool onlyPosition
)
{
    // compute sampling resolution
    int nU = static_cast<int>(1.0f / samplingRes) + 1;
    int nV = static_cast<int>(1.0f / samplingRes) + 1;

    // estimate vertex/triangle counts similar to CPU path
    int innerRows = std::max(0, nV - 4);
    size_t numVertices = 1 + static_cast<size_t>(innerRows) * static_cast<size_t>(nU) + 1;
    int innerQuadRows = std::max(0, nV - 5);
    size_t numTriangles = static_cast<size_t>(nU) + static_cast<size_t>(innerQuadRows) * static_cast<size_t>(nU) * 2 + static_cast<size_t>(nU);
    size_t numIndices = numTriangles * 3;

    // safety: avoid huge allocations on GPU
    const size_t MAX_VERTICES = 10'000'000;
    if (numVertices == 0 || numVertices > MAX_VERTICES) {
        return Geometry::Mesh::fromPlanet(planet, color, samplingRes);
    }

    // flatten control points
    auto cps = planet.controlPoints();
    std::vector<float> controlPointsData;
    controlPointsData.reserve(cps.size() * 3);
    for (const auto& p : cps) { controlPointsData.push_back(p.x); controlPointsData.push_back(p.y); controlPointsData.push_back(p.z); }

    // regenerate knots and basic planet metadata required by shader
    int cpPerParallel = static_cast<int>(planet.parallelsCP()[0].size());
    int parallelsCount = static_cast<int>(planet.parallelsCP().size());
    int degreeU = planet.degreeU();
    int degreeV = planet.degreeV();

    auto regeneratedKnotsU = BSpline::generateKnots(cpPerParallel, degreeU, 0);
    auto regeneratedKnotsV = BSpline::generateKnots(parallelsCount, degreeV, degreeV);
    std::vector<int> knotsUData(regeneratedKnotsU.begin(), regeneratedKnotsU.end());
    std::vector<int> knotsVData(regeneratedKnotsV.begin(), regeneratedKnotsV.end());

    // metal-cpp setup (same pattern used in Planet::isAutointersecating)
    auto device = NS::TransferPtr(MTL::CreateSystemDefaultDevice());
    if (!device) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);
    auto pool = NS::TransferPtr(NS::AutoreleasePool::alloc()->init());
    NS::Error* error = nullptr;
    auto libraryPath = NS::Bundle::mainBundle()->resourcePath()->stringByAppendingString(
        NS::String::string("/Shaders.metallib", NS::ASCIIStringEncoding)
    );
    auto library = NS::TransferPtr(device->newLibrary(libraryPath, &error));
    if (!library) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);

    // expected compute functions in metallib
    auto buildVerticesFunc = onlyPosition? NS::String::string("buildVerticesOnlyPosition", NS::ASCIIStringEncoding) : NS::String::string("buildVertices", NS::ASCIIStringEncoding);
    auto funcVertices = NS::TransferPtr(library->newFunction(buildVerticesFunc));
    auto funcIndices = NS::TransferPtr(library->newFunction(NS::String::string("buildIndices", NS::ASCIIStringEncoding)));
    if (!funcVertices || !funcIndices) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);

    NS::Error* errPSO = nullptr;
    auto pipelineVertices = NS::TransferPtr(device->newComputePipelineState(funcVertices.get(), &errPSO));
    if (!pipelineVertices) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);
    NS::Error* errPSO2 = nullptr;
    auto pipelineIndices = NS::TransferPtr(device->newComputePipelineState(funcIndices.get(), &errPSO2));
    if (!pipelineIndices) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);

    auto commandQueue = NS::TransferPtr(device->newCommandQueue());

    // allocate GPU buffers
    auto controlPointsBuffer = NS::TransferPtr(device->newBuffer(controlPointsData.data(), controlPointsData.size() * sizeof(float), MTL::ResourceStorageModeShared));
    auto knotsUBuffer = NS::TransferPtr(device->newBuffer(knotsUData.data(), knotsUData.size() * sizeof(int), MTL::ResourceStorageModeShared));
    auto knotsVBuffer = NS::TransferPtr(device->newBuffer(knotsVData.data(), knotsVData.size() * sizeof(int), MTL::ResourceStorageModeShared));

    auto outPositionsBuffer = NS::TransferPtr(device->newBuffer(numVertices * sizeof(float) * 3, MTL::ResourceStorageModeShared));
    NS::SharedPtr<MTL::Buffer> outNormalsBuffer = nullptr;
    if (!onlyPosition) outNormalsBuffer = NS::TransferPtr(device->newBuffer(numVertices * sizeof(float) * 3, MTL::ResourceStorageModeShared));
    NS::SharedPtr<MTL::Buffer> outTexcoordsBuffer = nullptr;
    if (!onlyPosition) outTexcoordsBuffer = NS::TransferPtr(device->newBuffer(numVertices * sizeof(float) * 2, MTL::ResourceStorageModeShared));
    auto outIndicesBuffer = NS::TransferPtr(device->newBuffer(numIndices * sizeof(uint32_t), MTL::ResourceStorageModeShared));

    if (!controlPointsBuffer || !knotsUBuffer || !knotsVBuffer || !outPositionsBuffer || !outIndicesBuffer) {
        return Geometry::Mesh::fromPlanet(planet, color, samplingRes);
    }
    if (!onlyPosition and (!outNormalsBuffer or !outTexcoordsBuffer)) return Geometry::Mesh::fromPlanet(planet, color, samplingRes);

    // params struct (must mirror the Metal struct layout)
    struct Params {
        int nU;
        int nV;
        int degreeU;
        int degreeV;
        int parallelsCount;
        int cpPerParallel;
        int knotsUSize;
        int knotsVSize;
        float samplingRes;
    } params{nU, nV, degreeU, degreeV, parallelsCount, cpPerParallel, static_cast<int>(knotsUData.size()), static_cast<int>(knotsVData.size()), samplingRes};

    auto paramsBuffer = NS::TransferPtr(device->newBuffer(&params, sizeof(Params), MTL::ResourceStorageModeShared));

    // dispatch buildVertices kernel
    auto commandBuffer = commandQueue->commandBuffer();
    auto encoder = commandBuffer->computeCommandEncoder();
    encoder->setComputePipelineState(pipelineVertices.get());
    encoder->setBuffer(controlPointsBuffer.get(), 0, 0);
    encoder->setBuffer(knotsUBuffer.get(), 0, 1);
    encoder->setBuffer(knotsVBuffer.get(), 0, 2);
    encoder->setBuffer(outPositionsBuffer.get(), 0, 3);
    if (!onlyPosition)
    {
        encoder->setBuffer(outNormalsBuffer.get(), 0, 4);
        encoder->setBuffer(outTexcoordsBuffer.get(), 0, 5);
    }
    encoder->setBuffer(paramsBuffer.get(), 0, 7);

    auto gridSize = MTL::Size::Make(static_cast<uint64_t>(numVertices), 1, 1);
    auto tg = std::min<uint64_t>(pipelineVertices->maxTotalThreadsPerThreadgroup(), 256);
    auto threadgroupSize = MTL::Size::Make(tg, 1, 1);
    encoder->dispatchThreads(gridSize, threadgroupSize);
    encoder->endEncoding();
    commandBuffer->commit();
    commandBuffer->waitUntilCompleted();

    // dispatch buildIndices kernel
    auto commandBuffer2 = commandQueue->commandBuffer();
    auto encoder2 = commandBuffer2->computeCommandEncoder();
    encoder2->setComputePipelineState(pipelineIndices.get());
    encoder2->setBuffer(paramsBuffer.get(), 0, 7);
    encoder2->setBuffer(outIndicesBuffer.get(), 0, 6);

    auto gridSize2 = MTL::Size::Make(static_cast<uint64_t>(numTriangles), 1, 1);
    auto tg2 = std::min<uint64_t>(pipelineIndices->maxTotalThreadsPerThreadgroup(), 256);
    auto threadgroupSize2 = MTL::Size::Make(tg2, 1, 1);
    encoder2->dispatchThreads(gridSize2, threadgroupSize2);
    encoder2->endEncoding();
    commandBuffer2->commit();
    commandBuffer2->waitUntilCompleted();

    // read back results
    auto posPtr = reinterpret_cast<float*>(outPositionsBuffer->contents());
    float* normPtr = nullptr;
    if (!onlyPosition) normPtr = reinterpret_cast<float*>(outNormalsBuffer->contents());
    float* texPtr = nullptr;
    if (!onlyPosition) texPtr = reinterpret_cast<float*>(outTexcoordsBuffer->contents());
    auto idxPtr = reinterpret_cast<uint32_t*>(outIndicesBuffer->contents());

    std::vector<glm::vec3> vertices(numVertices);
    std::vector<glm::vec3> normals;
    if (!onlyPosition) normals.resize(numVertices);
    std::vector<glm::vec2> texcoords;
    if (!onlyPosition) texcoords.resize(numVertices);
    std::vector<uint32_t> indices(numIndices);

    for (size_t i = 0; i < numVertices; ++i) {
        vertices[i] = glm::vec3(posPtr[i * 3 + 0], posPtr[i * 3 + 1], posPtr[i * 3 + 2]);
        if (!onlyPosition)
        {
            normals[i] = glm::vec3(normPtr[i * 3 + 0], normPtr[i * 3 + 1], normPtr[i * 3 + 2]);
            texcoords[i] = glm::vec2(texPtr[i * 2 + 0], texPtr[i * 2 + 1]);
        }
    }
    for (size_t i = 0; i < numIndices; ++i) indices[i] = idxPtr[i];

    // assemble vertexData like CPU path
    std::vector<Core::VertexAttributeName> attributeNames;
    std::vector<Core::VertexAttributeType> attributeTypes;
    if (!onlyPosition)
    {
        attributeNames = {
            Core::VertexAttributeName::Position,
            Core::VertexAttributeName::Color,
            Core::VertexAttributeName::Normal,
            Core::VertexAttributeName::TexCoord
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float4,
            Core::VertexAttributeType::Float3,
            Core::VertexAttributeType::Float2
        };
    } else
    {
        attributeNames = {
            Core::VertexAttributeName::Position
        };
        attributeTypes = {
            Core::VertexAttributeType::Float3
        };
    }

    std::vector<glm::vec4> colors;
    if (!onlyPosition) colors = std::vector(numVertices, color);

    std::vector<std::vector<uint8_t>> vertexData;
    if (!onlyPosition)
    {
        vertexData.resize(4);
        vertexData[1].resize(colors.size() * sizeof(glm::vec4)); std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
        vertexData[2].resize(normals.size() * sizeof(glm::vec3)); std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
        vertexData[3].resize(texcoords.size() * sizeof(glm::vec2)); std::memcpy(vertexData[3].data(), texcoords.data(), vertexData[3].size());
    } else
    {
        vertexData.resize(1);
    }
    vertexData[0].resize(vertices.size() * sizeof(glm::vec3)); std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetFitnessColor(
        const Planet& planet,
        const glm::vec4& c1,
        const glm::vec4& c2,
        float samplingRes,
        bool discreteColoring,
        float fitnessTreshold
        )
{
    auto mesh = Geometry::Mesh::fromPlanet(planet, c1, samplingRes);
    auto gc = GravityAdapter::GravityComputer(*mesh);

    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;
    std::vector<glm::vec2> textCoords;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);

    auto fitness = planet.fitness(0.0f, 0.0f, gc);
    if (discreteColoring)
    {
        if (fitness < fitnessTreshold)
        {
            colors.push_back(c1);
        }
        else
        {
            colors.push_back(c2);
        }
    }
    else
    {
        colors.push_back(glm::mix(c1, c2, fitness));
    }

    textCoords.emplace_back(0.0f, 0.0f);
    normals.emplace_back(glm::normalize(planet.normal(0.0f, 0.0f)));

    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1 + 1; i < nV - 1 - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));

            fitness = planet.fitness(u, v, gc);
            if (discreteColoring)
            {
                if (fitness < fitnessTreshold)
                {
                    colors.push_back(c1);
                }
                else
                {
                    colors.push_back(c2);
                }
            }
            else
            {
                colors.push_back(glm::mix(c1, c2, fitness));
            }
            textCoords.emplace_back(u, v);
            normals.emplace_back(glm::normalize(planet.normal(u, v)));
        }
    }

    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);

    fitness = planet.fitness(0.0f, 1.0f, gc);
    if (discreteColoring)
    {
        if (fitness < fitnessTreshold)
        {
            colors.push_back(c1);
        }
        else
        {
            colors.push_back(c2);
        }
    }
    else
    {
        colors.push_back(glm::mix(c1, c2, fitness));
    }
    textCoords.emplace_back(0.0f, 1.0f);
    normals.emplace_back(glm::normalize(planet.normal(0.0f, 1.0f)));

    // SUD POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(v1);
        indices.push_back(southPoleIdx);
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1 - 2; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v0);
            // Second Triangle
            indices.push_back(v3);
            indices.push_back(v2);
            indices.push_back(v1);
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    auto northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3 - 2) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v2);
        indices.push_back(northPoleIdx);
        indices.push_back(v1);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::Normal,
        Core::VertexAttributeName::TexCoord
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float2
    };
    std::vector<std::vector<uint8_t>> vertexData(4);
    vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    vertexData[2].resize(normals.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());
    vertexData[3].resize(textCoords.size() * sizeof(glm::vec2));
    std::memcpy(vertexData[3].data(), textCoords.data(), vertexData[3].size());

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetMeanCurvatureColor(
        const Planet& planet,
        const glm::vec4& c1,
        const glm::vec4& c2,
        float samplingRes
        )
{
    auto minCurvature = 100000.0f;
    auto maxCurvature = -100000.0f;
    
    auto u = 0.0f;
    while (u < 1.0f - 0.0f) {
        auto v = 0.1f;
        while (v <= 1.0f - 0.1f) {
            auto c = planet.meanCurvature(u, v);

            if (c > maxCurvature) maxCurvature = c;
            if (c < minCurvature) minCurvature = c;
            v += samplingRes;
        }
        u += samplingRes;
    }
    
    auto range = std::abs(maxCurvature - minCurvature);
    if (range == 0) range = 1;
    
    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);
    
    auto mc = planet.meanCurvature(0.0f, 0.0f);
    glm::vec4 color;
    if (mc >= minCurvature or mc <= maxCurvature)
        color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    else
        color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);

    colors.push_back(color);
    /*
    if (planet.meanCurvature(0.0f, 0.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 0.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);
    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1 + 1; i < nV - 1 - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));

            mc = planet.meanCurvature(u, v);
            //std::cout << "curvature for " << u << " " << v << ": " << mc << std::endl;
            if (mc >= minCurvature or mc <= maxCurvature)
                color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
            else
                color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            colors.push_back(c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range);
            /*
            if (planet.meanCurvature(u, v) < 0.0f)
            {
                colors.push_back(c2);
            }
            else
            {
                colors.push_back(c1);
            }
             */
            
            //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(u, v, gc)));
            // temporary init
            normals.emplace_back(0.0f);
        }
    }
    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);

    mc = planet.meanCurvature(0.0f, 1.0f);
    if (mc >= minCurvature or mc <= maxCurvature)
        color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    else
        color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    colors.push_back(c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range);
    /*
    if (planet.meanCurvature(0.0f, 1.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 1.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);

    // NORTH POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(southPoleIdx);
        indices.push_back(v1);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[v1] - vertices[southPoleIdx], vertices[v2] - vertices[southPoleIdx]));
        normals[southPoleIdx] = n;
        normals[v1] = n;
        normals[v2] = n;
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1 - 2; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);
            glm::vec3 n1 = glm::normalize(glm::cross(vertices[v2] - vertices[v0], vertices[v1] - vertices[v0]));
            normals[v0] += n1; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n1; normals[v1] = glm::normalize(normals[v1]);
            normals[v1] += n1; normals[v2] = glm::normalize(normals[v2]);
            // Second Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
            glm::vec3 n2 = glm::normalize(glm::cross(vertices[v2] - vertices[v1], vertices[v3] - vertices[v1]));
            normals[v1] += n2; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n2; normals[v1] = glm::normalize(normals[v1]);
            normals[v3] += n2; normals[v2] = glm::normalize(normals[v2]);
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    uint32_t northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3 - 2) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v1);
        indices.push_back(northPoleIdx);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[northPoleIdx] - vertices[v1], vertices[v2] - vertices[v1]));
        normals[northPoleIdx] += n; normals[northPoleIdx] = glm::normalize(normals[northPoleIdx]);
        normals[v1] += n; normals[v1] = glm::normalize(normals[v1]);
        normals[v2] += n; normals[v2] = glm::normalize(normals[v2]);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::Normal,
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float3
    };
    std::vector<std::vector<uint8_t>> vertexData(3);
    vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    vertexData[2].resize(normals.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetGaussCurvatureColor(
        const Planet& planet,
        const glm::vec4& c1,
        const glm::vec4& c2,
        float samplingRes
        )
{
    auto minCurvature = 100000.0f;
    auto maxCurvature = -100000.0f;

    auto u = 0.0f;
    while (u < 1.0f - 0.0f) {
        auto v = 0.1f;
        while (v <= 1.0f - 0.1f) {
            auto c = planet.gaussCurvature(u, v);

            if (c > maxCurvature) maxCurvature = c;
            if (c < minCurvature) minCurvature = c;
            v += samplingRes;
        }
        u += samplingRes;
    }

    auto range = std::abs(maxCurvature - minCurvature);
    if (range == 0) range = 1;

    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);

    auto mc = planet.gaussCurvature(0.0f, 0.0f);
    glm::vec4 color;
    if (mc >= minCurvature or mc <= maxCurvature)
        color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    else
        color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    colors.push_back(color);
    /*
    if (planet.meanCurvature(0.0f, 0.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 0.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);
    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1 + 1; i < nV - 1 - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));

            mc = planet.gaussCurvature(u, v);
            //std::cout << "curvature for " << u << " " << v << ": " << mc << std::endl;
            if (mc >= minCurvature or mc <= maxCurvature)
                color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
            else
                color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
            colors.push_back(color);
            /*
            if (planet.meanCurvature(u, v) < 0.0f)
            {
                colors.push_back(c2);
            }
            else
            {
                colors.push_back(c1);
            }
             */

            //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(u, v, gc)));
            // temporary init
            normals.emplace_back(0.0f);
        }
    }
    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);

    mc = planet.gaussCurvature(0.0f, 1.0f);
    if (mc >= minCurvature or mc <= maxCurvature)
        color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    else
        color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
    colors.push_back(color);
    /*
    if (planet.meanCurvature(0.0f, 1.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 1.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);

    // NORTH POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(southPoleIdx);
        indices.push_back(v1);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[v1] - vertices[southPoleIdx], vertices[v2] - vertices[southPoleIdx]));
        normals[southPoleIdx] = n;
        normals[v1] = n;
        normals[v2] = n;
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1 - 2; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);
            glm::vec3 n1 = glm::normalize(glm::cross(vertices[v2] - vertices[v0], vertices[v1] - vertices[v0]));
            normals[v0] += n1; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n1; normals[v1] = glm::normalize(normals[v1]);
            normals[v1] += n1; normals[v2] = glm::normalize(normals[v2]);
            // Second Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
            glm::vec3 n2 = glm::normalize(glm::cross(vertices[v2] - vertices[v1], vertices[v3] - vertices[v1]));
            normals[v1] += n2; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n2; normals[v1] = glm::normalize(normals[v1]);
            normals[v3] += n2; normals[v2] = glm::normalize(normals[v2]);
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    uint32_t northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3 - 2) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v1);
        indices.push_back(northPoleIdx);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[northPoleIdx] - vertices[v1], vertices[v2] - vertices[v1]));
        normals[northPoleIdx] += n; normals[northPoleIdx] = glm::normalize(normals[northPoleIdx]);
        normals[v1] += n; normals[v1] = glm::normalize(normals[v1]);
        normals[v2] += n; normals[v2] = glm::normalize(normals[v2]);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::Normal
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float3
    };
    std::vector<std::vector<uint8_t>> vertexData(3);
    vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    vertexData[2].resize(normals.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::shared_ptr<Geometry::Mesh> Geometry::Mesh::fromPlanetLaplacianCurvatureColor(
        const Planet& planet,
        const glm::vec4& c1,
        const glm::vec4& c2,
        float samplingRes
        )
{
    auto minCurvature = 100000.0f;
    auto maxCurvature = -100000.0f;

    auto u = 0.0f;
    while (u < 1.0f - 0.0f) {
        auto v = 0.0f;
        while (v <= 1.0f - 0.0f) {
            auto c = planet.laplacianCurvature(u, v);

            if (c > maxCurvature) maxCurvature = c;
            if (c < minCurvature) minCurvature = c;
            v += samplingRes;
        }
        u += samplingRes;
    }

    auto range = std::abs(maxCurvature - minCurvature);
    if (range == 0) range = 1;

    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);

    auto mc = planet.laplacianCurvature(0.0f, 0.0f);
    auto color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    colors.push_back(color);
    /*
    if (planet.meanCurvature(0.0f, 0.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 0.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);
    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1 + 1; i < nV - 1 - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));

            mc = planet.laplacianCurvature(u, v);
            //std::cout << "curvature for " << u << " " << v << ": " << mc << std::endl;
            color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
            colors.push_back(c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range);
            /*
            if (planet.meanCurvature(u, v) < 0.0f)
            {
                colors.push_back(c2);
            }
            else
            {
                colors.push_back(c1);
            }
             */

            //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(u, v, gc)));
            // temporary init
            normals.emplace_back(0.0f);
        }
    }
    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);

    mc = planet.laplacianCurvature(0.0f, 1.0f);
    color = c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range;
    colors.push_back(c1 * (maxCurvature - mc) / range + c2 * (mc - minCurvature) / range);
    /*
    if (planet.meanCurvature(0.0f, 1.0f) < 0.0f)
    {
        colors.push_back(c2);
    }
    else
    {
        colors.push_back(c1);
    }
     */
    //colors.push_back(glm::mix(c1, c2, 1 - planet.fitness(0.0f, 1.0f, gc)));
    // temporary init
    normals.emplace_back(0.0f);

    // NORTH POLE FAN
    uint32_t southPoleIdx = 0;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = 1 + j;
        uint32_t v2 = 1 + ((j + 1) % nU);
        indices.push_back(southPoleIdx);
        indices.push_back(v1);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[v1] - vertices[southPoleIdx], vertices[v2] - vertices[southPoleIdx]));
        normals[southPoleIdx] = n;
        normals[v1] = n;
        normals[v2] = n;
    }
    // INNER PARALLELS
    // additional -2 from nV for first ring skip
    for (int i = 0; i < nV - 2 - 1 - 2; ++i) { // nV-2 parallels, -1 for avoiding overflow
        for (int j = 0; j < nU; ++j) {
            uint32_t row0 = 1 + i * nU;
            uint32_t row1 = 1 + (i + 1) * nU;
            uint32_t v0 = row0 + j;
            uint32_t v1 = row0 + ((j + 1) % nU);
            uint32_t v2 = row1 + j;
            uint32_t v3 = row1 + ((j + 1) % nU);
            // First Triangle
            indices.push_back(v0);
            indices.push_back(v2);
            indices.push_back(v1);
            glm::vec3 n1 = glm::normalize(glm::cross(vertices[v2] - vertices[v0], vertices[v1] - vertices[v0]));
            normals[v0] += n1; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n1; normals[v1] = glm::normalize(normals[v1]);
            normals[v1] += n1; normals[v2] = glm::normalize(normals[v2]);
            // Second Triangle
            indices.push_back(v1);
            indices.push_back(v2);
            indices.push_back(v3);
            glm::vec3 n2 = glm::normalize(glm::cross(vertices[v2] - vertices[v1], vertices[v3] - vertices[v1]));
            normals[v1] += n2; normals[v0] = glm::normalize(normals[v0]);
            normals[v2] += n2; normals[v1] = glm::normalize(normals[v1]);
            normals[v3] += n2; normals[v2] = glm::normalize(normals[v2]);
        }
    }
    // Sud: fan
    // -2 from nV for first ring skip
    uint32_t northPoleIdx = static_cast<uint32_t>(vertices.size() - 1);
    uint32_t lastRow = 1 + (nV - 3 - 2) * nU;
    for (int j = 0; j < nU; ++j) {
        uint32_t v1 = lastRow + j;
        uint32_t v2 = lastRow + ((j + 1) % nU);
        indices.push_back(v1);
        indices.push_back(northPoleIdx);
        indices.push_back(v2);
        glm::vec3 n = glm::normalize(glm::cross(vertices[northPoleIdx] - vertices[v1], vertices[v2] - vertices[v1]));
        normals[northPoleIdx] += n; normals[northPoleIdx] = glm::normalize(normals[northPoleIdx]);
        normals[v1] += n; normals[v1] = glm::normalize(normals[v1]);
        normals[v2] += n; normals[v2] = glm::normalize(normals[v2]);
    }

    // Attributi
    std::vector<Core::VertexAttributeName> attributeNames = {
        Core::VertexAttributeName::Position,
        Core::VertexAttributeName::Color,
        Core::VertexAttributeName::Normal
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float3
    };
    std::vector<std::vector<uint8_t>> vertexData(3);
    vertexData[0].resize(vertices.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[0].data(), vertices.data(), vertexData[0].size());
    vertexData[1].resize(colors.size() * sizeof(glm::vec4));
    std::memcpy(vertexData[1].data(), colors.data(), vertexData[1].size());
    vertexData[2].resize(normals.size() * sizeof(glm::vec3));
    std::memcpy(vertexData[2].data(), normals.data(), vertexData[2].size());

    return std::make_shared<Mesh>(
        static_cast<int>(vertices.size()),
        static_cast<int>(indices.size() / 3),
        attributeNames,
        attributeTypes,
        vertexData,
        indices
    );
}

std::vector<glm::vec3> Geometry::Mesh::getVertices() const
{
    std::vector<glm::vec3> vertices;
    // Trova l'indice dell'attributo posizione
    int posIndex = -1;
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i) {
        if (_vertexAttributeNames[i] == Core::VertexAttributeName::Position) {
            posIndex = static_cast<int>(i);
            break;
        }
    }
    if (posIndex == -1) return vertices;
    const std::vector<uint8_t>& posData = _vertexData[posIndex];
    size_t numVerts = posData.size() / (sizeof(float) * 3);
    vertices.reserve(numVerts);
    for (size_t i = 0; i < numVerts; ++i) {
        float x = *reinterpret_cast<const float*>(&posData[i * 3 * sizeof(float) + 0 * sizeof(float)]);
        float y = *reinterpret_cast<const float*>(&posData[i * 3 * sizeof(float) + 1 * sizeof(float)]);
        float z = *reinterpret_cast<const float*>(&posData[i * 3 * sizeof(float) + 2 * sizeof(float)]);
        vertices.emplace_back(x, y, z);
    }
    return vertices;
}

std::vector<glm::vec3> Geometry::Mesh::getTriangles() const
{
    std::vector<glm::vec3> triangles;
    auto vertices = getVertices();
    for (size_t i = 0; i + 2 < _faces.size(); i += 3) {
        triangles.push_back(vertices[_faces[i]]);
        triangles.push_back(vertices[_faces[i + 1]]);
        triangles.push_back(vertices[_faces[i + 2]]);
    }
    return triangles;
}

glm::vec2 Geometry::Mesh::uvFromRay(glm::vec3 origin, glm::vec3 direction) const
{
    // Normalizza la direzione del raggio
    direction = glm::normalize(direction);
    
    // Verifica che la mesh abbia le coordinate texture
    if (!HasAttribute(Core::VertexAttributeName::TexCoord)) {
        return glm::vec2(-1.0f, -1.0f); // Restituisce valore non valido
    }
    
    // Ottieni i dati dei vertici, delle coordinate UV e degli indici
    auto vertices = getVertices();
    const auto& uvData = getAttributeData(Core::VertexAttributeName::TexCoord);
    const auto& indices = _faces;
    
    // Converti i dati UV da byte array a vettore di glm::vec2
    std::vector<glm::vec2> uvCoords;
    size_t numUVs = uvData.size() / (sizeof(float) * 2);
    uvCoords.reserve(numUVs);
    
    for (size_t i = 0; i < numUVs; ++i) {
        float u = *reinterpret_cast<const float*>(&uvData[i * 2 * sizeof(float) + 0 * sizeof(float)]);
        float v = *reinterpret_cast<const float*>(&uvData[i * 2 * sizeof(float) + 1 * sizeof(float)]);
        uvCoords.emplace_back(u, v);
    }
    
    float closestDistance = std::numeric_limits<float>::max();
    glm::vec2 resultUV(-1.0f, -1.0f);
    
    // Itera attraverso tutti i triangoli della mesh
    for (size_t i = 0; i < indices.size(); i += 3) {
        // Ottieni i tre vertici del triangolo
        uint32_t idx0 = indices[i];
        uint32_t idx1 = indices[i + 1];
        uint32_t idx2 = indices[i + 2];
        
        glm::vec3 v0 = vertices[idx0];
        glm::vec3 v1 = vertices[idx1];
        glm::vec3 v2 = vertices[idx2];
        
        // Usa la funzione di intersezione dalla libreria gravity
        float t;
        if (util::ray_triangle_intersection(origin, direction, v0, v1, v2, &t)) {
            if (t > 0.00001f && t < closestDistance) { // Intersezione valida
                closestDistance = t;
                
                // Calcola il punto di intersezione
                glm::vec3 intersectionPoint = origin + direction * t;
                
                // Calcola le coordinate baricentriche usando la funzione utility esistente
                glm::vec3 baryCoords = util::barycentric_coords(v0, v1, v2, intersectionPoint);
                
                // Interpola le coordinate UV usando le coordinate baricentriche
                glm::vec2 uv0 = uvCoords[idx0];
                glm::vec2 uv1 = uvCoords[idx1];
                glm::vec2 uv2 = uvCoords[idx2];
                
                resultUV = baryCoords.x * uv0 + baryCoords.y * uv1 + baryCoords.z * uv2;
            }
        }
    }
    
    return resultUV;
}

std::pair<bool, glm::vec3> Geometry::Mesh::rayIntersection(glm::vec3 origin, glm::vec3 direction) const
{
    direction = glm::normalize(direction);

    auto vertices = getVertices();
    const auto& indices = _faces;

    float closestDistance = std::numeric_limits<float>::max();
    bool found;

    for (size_t i = 0; i < indices.size(); i += 3) {
        uint32_t idx0 = indices[i];
        uint32_t idx1 = indices[i + 1];
        uint32_t idx2 = indices[i + 2];

        glm::vec3 v0 = vertices[idx0];
        glm::vec3 v1 = vertices[idx1];
        glm::vec3 v2 = vertices[idx2];

        float t;
        if (util::ray_triangle_intersection(origin, direction, v0, v1, v2, &t)) {
            if (t > 0.00001f && t < closestDistance) {
                found = true;
                closestDistance = t;
            }
        }
    }
    return {found, origin + closestDistance * direction};
}

std::vector<glm::vec3> Geometry::Mesh::getNormals() const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == Core::VertexAttributeName::Normal)
        {
            auto byteNormals = _vertexData[i];
            auto vecNormals = std::vector<glm::vec3>(_numVertices);
            for (size_t j = 0; j < _numVertices; ++j)
            {
                const auto* fdata = reinterpret_cast<const float*>(byteNormals.data());
                vecNormals[j] = glm::vec3(fdata[j * 3], fdata[j * 3 + 1], fdata[j * 3 + 2]);
            }
            return vecNormals;
        }
    }
    throw std::runtime_error("Mesh does not contain normals");
}

std::vector<glm::vec2> Geometry::Mesh::getTextureCoordinated() const
{
    for (size_t i = 0; i < _vertexAttributeNames.size(); ++i)
    {
        if (_vertexAttributeNames[i] == Core::VertexAttributeName::TexCoord)
        {
            auto byteUV = _vertexData[i];
            auto vecUV = std::vector<glm::vec2>(_numVertices);
            for (size_t j = 0; j < _numVertices; ++j)
            {
                const auto* fdata = reinterpret_cast<const float*>(byteUV.data());
                vecUV[j] = glm::vec2(fdata[j * 2], fdata[j * 2 + 1]);
            }
            return vecUV;
        }
    }
    throw std::runtime_error("Mesh does not contain texture coordinates");
}

std::vector<Core::VertexAttributeName> Geometry::Mesh::getAttributes() const
{
    return _vertexAttributeNames;
}

[[nodiscard]] glm::vec4 Geometry::Mesh::boundingSphere() const
{
    auto center = glm::vec3(0.0f);
    float radius = 0.0f;
    for (const auto& v : getVertices()) {
        center += v;
    }
    center /= static_cast<float>(getVertices().size());
    for (const auto& v : getVertices()) {
        radius = std::max(radius, glm::distance(center, v));
    }
    return {center, radius};
}

