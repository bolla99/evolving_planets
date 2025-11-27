//
// Created by Giovanni Bollati on 11/06/25.
//
#include "Mesh.hpp"
#include <BSpline.hpp>
#include <genetic.hpp>



Mesh::Mesh(int numVertices, int numFaces,
     const std::vector<Core::VertexAttributeName>& vertexAttributeNames,
     const std::vector<Core::VertexAttributeType>& vertexAttributeTypes,
     const std::vector<std::vector<uint8_t>>& vertexData,
     const std::vector<uint32_t>& faces) : _numVertices(numVertices),
                                                            _numFaces(numFaces),
                                                            _vertexAttributeNames(vertexAttributeNames),
                                                            _vertexAttributeTypes(vertexAttributeTypes),
                                                            _vertexData(vertexData),
                                                            _faces(faces) {}

bool Mesh::HasAttribute(Core::VertexAttributeName attributeName) const
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

Core::VertexAttributeType Mesh::GetAttributeType(Core::VertexAttributeName attributeName) const
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

const std::vector<uint8_t>& Mesh::getAttributeData(Core::VertexAttributeName attributeName) const
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

const std::vector<uint32_t>& Mesh::getFacesData() const
{
    return _faces;
}

std::shared_ptr<Mesh> Mesh::quad(
        const float* pos, const float w, const float h, const float* color, float uvWidth, float uvHeight
        )
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
        pos[0], pos[1], pos[2], // v0
        pos[0] + w, pos[1], pos[2], // v1
        pos[0] + w, pos[1] + h, pos[2], // v2
        pos[0], pos[1] + h, pos[2] // v3
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
        0, 1, 2, // First triangle
        0, 2, 3 // Second triangle
    };

    return std::make_shared<Mesh>(4, 2, attributeNames, attributeTypes, vertexData, faces);
}

std::shared_ptr<Mesh> Mesh::fromBSpline(
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

std::shared_ptr<Mesh> Mesh::fromPolygon(
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
    for (int i = 0; i < positions.size() - 1; ++i)
    {
        lines.push_back(positions[i]);
        if (addInnerVertices) lines.push_back(positions[i + 1]);
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


std::shared_ptr<Mesh> Mesh::fromPlanet(
    const Planet& planet,
    const glm::vec4& color,
    float samplingRes
)
{
    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<glm::vec2> textCoords;
    std::vector<uint32_t> indices;

    // SOUTH POLE
    glm::vec3 southPole = planet.evaluate(0.0f, 0.0f);
    vertices.push_back(southPole);
    textCoords.push_back(glm::vec2(0.0f, 0.0f));
    colors.push_back(color);
    // temporary init
    normals.emplace_back(0.0f);
    // Cintura centrale
    // skip one ring of the plateau, connect only to the outer
    for (int i = 1 + 1; i < nV - 1 - 1; ++i) { // +1 and -1 for first ring skip
        float v = static_cast<float>(i) / static_cast<float>(nV - 1);
        for (int j = 0; j < nU; ++j) {
            float u = static_cast<float>(j) / static_cast<float>(nU); // u in [0, 1) -> for linking
            vertices.push_back(planet.evaluate(u, v));
            textCoords.push_back(glm::vec2(u, v));
            colors.push_back(color);
            // temporary init
            normals.emplace_back(0.0f);
        }
    }
    // NORTH POLE
    glm::vec3 northPole = planet.evaluate(0.0f, 1.0f);
    vertices.push_back(northPole);
    textCoords.push_back(glm::vec2(0.0f, 1.0f));
    colors.push_back(color);
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
        Core::VertexAttributeName::TexCoord
    };
    std::vector<Core::VertexAttributeType> attributeTypes = {
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float4,
        Core::VertexAttributeType::Float3,
        Core::VertexAttributeType::Float2
    };
    std::vector<std::vector<uint8_t>> vertexData(4); // Changed from 3 to 4
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

std::shared_ptr<Mesh> Mesh::fromPlanetFitnessColor(
        const Planet& planet,
        const glm::vec4& c1,
        const glm::vec4& c2,
        float samplingRes,
        bool discreteColoring,
        float fitnessTreshold
        )
{
    auto mesh = Mesh::fromPlanet(planet, c1, samplingRes);
    auto gc = GravityAdapter::GravityComputer(*mesh);

    int nU = static_cast<int>(1.0f / samplingRes) + 1; // non includo u=1, per periodicità
    int nV = static_cast<int>(1.0f / samplingRes) + 1; // includo v=0 e v=1
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec4> colors;
    std::vector<glm::vec3> normals;
    std::vector<uint32_t> indices;

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

    // temporary init
    normals.emplace_back(0.0f);
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
            // temporary init
            normals.emplace_back(0.0f);
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

std::shared_ptr<Mesh> Mesh::fromPlanetMeanCurvatureColor(
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

std::shared_ptr<Mesh> Mesh::fromPlanetGaussCurvatureColor(
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

std::shared_ptr<Mesh> Mesh::fromPlanetLaplacianCurvatureColor(
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


std::vector<glm::vec3> Mesh::getVertices() const
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

std::vector<glm::vec3> Mesh::getTriangles() const
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

glm::vec2 Mesh::uvFromRay(glm::vec3 origin, glm::vec3 direction) const
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

glm::vec3 Mesh::rayIntersection(glm::vec3 origin, glm::vec3 direction) const
{
    direction = glm::normalize(direction);

    auto vertices = getVertices();
    const auto& indices = _faces;

    float closestDistance = std::numeric_limits<float>::max();

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
                closestDistance = t;
            }
        }
    }
    return origin + closestDistance * direction;
}

std::vector<glm::vec3> Mesh::getNormals() const
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

std::vector<glm::vec2> Mesh::getTextureCoordinated() const
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
