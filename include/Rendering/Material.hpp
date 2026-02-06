//
// Created by Giovanni Bollati on 30/01/26.
//

#ifndef EVOLVING_PLANETS_MATERIAL_HPP
#define EVOLVING_PLANETS_MATERIAL_HPP
#include "glm/vec4.hpp"
#include <exception>

enum MaterialType
{
    VCPHONG,
};

template <class T> struct MaterialTraits;

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

// ABSTRACT CLASS
struct IMaterial
{
    virtual ~IMaterial() = default;
    virtual size_t size() const = 0;
    const virtual void* bytes() const = 0;

    MaterialInfo info{};

    static std::shared_ptr<IMaterial> factory(MaterialInfo info);

    template <typename T>
    static std::shared_ptr<T> get(const std::shared_ptr<IMaterial>& mat)
    {
        if (mat == nullptr) return nullptr;
        if (mat->info.type == MaterialTraits<T>::type)
        {
            return static_pointer_cast<T>(mat);
        }
        else return nullptr;
    }
};

// MATERIALS
struct MatVCPHONG : public IMaterial
{
    struct Data
    {
        uint8_t addTint = 0;
        uint8_t padding[15]{};
        glm::vec4 tint = glm::vec4{1.0f, 1.0f, 1.0f, 1.0f};
    };
    Data data;

    [[nodiscard]] size_t size() const override { return sizeof(Data); }
    [[nodiscard]] const void* bytes() const override { return &data; }
};
template <> struct MaterialTraits<MatVCPHONG> { static constexpr MaterialType type = MaterialType::VCPHONG; };

inline std::shared_ptr<IMaterial> IMaterial::factory(const MaterialInfo info)
{
    switch (info.type)
    {
    case VCPHONG:
        {
            auto material = std::make_shared<MatVCPHONG>();
            material->info = info;
            return material;
        }
    default: throw std::runtime_error("Unknown material type");
    }
}



#endif //EVOLVING_PLANETS_MATERIAL_HPP
