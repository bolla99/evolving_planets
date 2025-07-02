//
// Created by Giovanni Bollati on 26/06/25.
//

#ifndef TEXTURE_HPP
#define TEXTURE_HPP
#include <vector>

#include "glm/vec4.hpp"

enum PixelFormat
{
    RGBA8,
    RGB8,
    R8
};

class Texture
{
public:
    Texture(
        const std::vector<uint8_t>& data,
        size_t width,
        size_t height,
        PixelFormat format,
        size_t bytesPerPixel
        );

    Texture(const Texture& otehr) = delete;
    Texture(Texture&& other) = delete;
    Texture& operator=(const Texture& other) = delete;
    Texture& operator=(Texture&& other) = delete;

    [[nodiscard]] const std::vector<uint8_t>& getData() const { return _data; }
    [[nodiscard]] size_t width() const { return _width; }
    [[nodiscard]] size_t height() const { return _height; }
    [[nodiscard]] PixelFormat format() const { return _format; }
    [[nodiscard]] size_t bytesPerPixel() const { return _bytesPerPixel; }

    static std::shared_ptr<Texture> fromFile(const std::string& filePath);
    static std::shared_ptr<Texture> fromText(const std::string& text, int fontSize, const glm::vec4& color);

private:
    std::vector<uint8_t> _data;
    size_t _width;
    size_t _height;
    PixelFormat _format;
    size_t _bytesPerPixel;
};

#endif //TEXTURE_HPP
