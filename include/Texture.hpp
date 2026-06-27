//
// Created by Giovanni Bollati on 26/06/25.
//

#ifndef TEXTURE_HPP
#define TEXTURE_HPP


#include <Rendering/Material.hpp>
#include <compare>

namespace Core
{
    enum PixelFormat
    {
        RGBA8,
        RGB8,
        R8,
        RGBA32F,
        R32F
    };

    enum TextureType
    {
        Diffuse,
        NormalMap,
        BSplineTexture,
        RockyNoise,
        RockyNoise_TL,
        RockyNoise_TR,
        RockyNoise_BL,
        RockyNoise_BR,
        Text,
        ShadowMap,
        CubeShadowMap,
        Billboard,
        OpaqueDepth,
        Potential3D,
        AtmosphereDensity3D,
        LightTransmittance3D
    };

    enum FilterType
    {
        Nearest,
        Linear
    };
    enum AddressMode
    {
        None,
        Repeat,
        ClampToEdge,
        ClampToZero,
        MirrorRepeat
    };

    struct TextureDescriptor
    {
        TextureType type;
        MaterialStage stage;
        int bufferID;
    };

    struct SamplerDescriptor {
        AddressMode addressModeU;
        AddressMode addressModeV;
        AddressMode addressModeW;
        FilterType minFilter;
        FilterType magFilter;
        bool normalizedCoordinates;

        auto operator<=>(const SamplerDescriptor&) const = default;
    };

    struct SamplerBinding
    {
        SamplerDescriptor descriptor;
        MaterialStage stage;
        int bufferID;
    };

    class Texture
    {
    public:
        Texture(
            const std::vector<uint8_t>& data,
            size_t width,
            size_t height,
            PixelFormat format,
            size_t bytesPerPixel,
            TextureType type,
            bool is3D = false,
            size_t depth = 1
            );

        Texture(const Texture& otehr) = delete;
        Texture(Texture&& other) = delete;
        Texture& operator=(const Texture& other) = delete;
        Texture& operator=(Texture&& other) = delete;

        [[nodiscard]] const std::vector<uint8_t>& getData() const { return _data; }
        [[nodiscard]] size_t width() const { return _width; }
        [[nodiscard]] size_t height() const { return _height; }
        [[nodiscard]] size_t depth() const { return _depth; }
        [[nodiscard]] bool is3D() const { return _is3D; }
        [[nodiscard]] PixelFormat format() const { return _format; }
        [[nodiscard]] size_t bytesPerPixel() const { return _bytesPerPixel; }
        [[nodiscard]] TextureType type() const { return _type; }

        void updateData(const std::vector<uint8_t>& data) { _data = data; }
        std::vector<uint8_t>& getData() { return _data; }

        [[nodiscard]] glm::vec4 sample(float u, float v) const;
        [[nodiscard]] glm::vec4 sample(float u, float v, float w) const;

        static std::shared_ptr<Texture> fromFile(const std::string& filePath, TextureType type);
        static std::shared_ptr<Texture> fromText(const std::string& text, int fontSize, const glm::vec4& color);
        static std::shared_ptr<Texture> fromColor(const glm::vec4& color);

    private:
        std::vector<uint8_t> _data;
        size_t _width;
        size_t _height;
        size_t _depth;
        bool _is3D;
        PixelFormat _format;
        size_t _bytesPerPixel;
        TextureType _type;
    };
}

#endif //TEXTURE_HPP
