//
// Created by Giovanni Bollati on 30/01/26.
//

#ifndef EVOLVING_PLANETS_MATERIAL_HPP
#define EVOLVING_PLANETS_MATERIAL_HPP
#include "glm/vec4.hpp"
#include <exception>

enum MaterialType
{
    TintType,
    RectType,
    ViewportSizeType
};

enum MaterialStage
{
    Vertex,
    Fragment
};
enum MaterialFrequency
{
    PerObject,
    PerFrame
};

struct MaterialInfo
{
    MaterialType type;
    MaterialStage stage;
    MaterialFrequency frequency;
    int bufferIndex;
};

// MATERIALS
struct Tint
{
    uint8_t addTint = 0;
    uint8_t _padding[15]{};
    glm::vec4 tintColor = glm::vec4(1.0f);
};

struct RectMaterial
{
    glm::vec4 rect = {0.0f, 0.0f, 100.0f, 100.0f};
};

struct ViewportSize
{
    float width = 100.0f;
    float height = 100.0f;
    // padding for 16byte alignment
    float padding1 = 0.0f;
    float padding2 = 0.0f;
};

constexpr std::vector<std::byte> getDefaultBytes(MaterialType type)
{
    switch (type)
    {
    case TintType:
        {
            auto bytes = std::vector<std::byte>(sizeof(Tint));
            constexpr auto material = Tint();
            std::memcpy(bytes.data(), &material, sizeof(Tint));
            return bytes;
        }
    case RectType:
        {
            auto bytes = std::vector<std::byte>(sizeof(RectMaterial));
            constexpr auto material = RectMaterial();
            std::memcpy(bytes.data(), &material, sizeof(RectMaterial));
            return bytes;
        }
    case ViewportSizeType:
        {
            auto bytes = std::vector<std::byte>(sizeof(ViewportSize));
            constexpr auto material = ViewportSize();
            std::memcpy(bytes.data(), &material, sizeof(ViewportSize));
            return bytes;
        }
    }
    throw std::runtime_error("Invalid material type");
}


#endif //EVOLVING_PLANETS_MATERIAL_HPP
