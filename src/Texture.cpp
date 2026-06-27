//
// Created by Giovanni Bollati on 26/06/25.
//

#include <Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#include <SDL_ttf.h>
#include <simd/packed.h>
#include <glm/glm.hpp>

#include "Apple/Util.hpp"

using namespace Core;

Texture::Texture(
        const std::vector<uint8_t>& data,
        size_t width,
        size_t height,
        PixelFormat format,
        size_t bytesPerPixel,
        TextureType type,
        bool is3D,
        size_t depth
        ) :   _data(data),
              _width(width),
              _height(height),
              _format(format),
              _bytesPerPixel(bytesPerPixel),
              _type(type),
              _is3D(is3D),
              _depth(depth)
{}


std::shared_ptr<Texture> Texture::fromFile(const std::string& filePath, TextureType type)
{
    int width, height, channels;
    auto data = stbi_load(filePath.c_str(), &width, &height, &channels, 0);
    if (!data) {
        throw std::runtime_error("Failed to load image: " + std::string(stbi_failure_reason()));
    }
    try
    {
        PixelFormat format;
        switch (channels)
        {
        case 1: format = R8; break;
        case 3: format = RGB8; break;
        case 4: format = RGBA8; break;
        default: throw std::runtime_error("Unsupported number of channels: " + std::to_string(channels));
        };
        auto texture = std::make_shared<Texture>(
            std::vector<uint8_t>(data, data + width * height * channels),
            static_cast<size_t>(width),
            static_cast<size_t>(height),
            format,
            channels,
            type
        );
        stbi_image_free(data);
        return texture;
    } catch (const std::exception& e)
    {
        stbi_image_free(data);
        throw std::runtime_error("Error creating texture: " + std::string(e.what()));
    }
}

std::shared_ptr<Texture> Texture::fromText(const std::string& text, int fontSize, const glm::vec4& color)
{
    TTF_Init();
    TTF_Font* font = TTF_OpenFont(Apple::resourcePath("OpenSans-Regular.ttf").c_str(), fontSize);
    if (!font) {
        TTF_Quit();
        throw std::runtime_error("Failed to load font: " + std::string(TTF_GetError()));
    }


    auto surface = TTF_RenderUTF8_Blended(font, text.c_str(), {
        static_cast<uint8_t>(255.0f * color.x),
        static_cast<uint8_t>(255.0f * color.y),
        static_cast<uint8_t>(255.0f * color.z), 255}
        );
    surface = SDL_ConvertSurfaceFormat(surface, SDL_PIXELFORMAT_RGBA32, 0);


    //SDL_Log("FORMAT: %s", surface->format);
    TTF_CloseFont(font);
    TTF_Quit();

    if (!surface) {
        throw std::runtime_error("Failed to create surface from text: " + std::string(TTF_GetError()));
    }

    auto texture = std::make_shared<Texture>(
        std::vector(static_cast<uint8_t*>(surface->pixels), static_cast<uint8_t*>(surface->pixels) + surface->w * surface->h * 4),
        static_cast<size_t>(surface->w),
        static_cast<size_t>(surface->h),
        RGBA8,
        4,
        TextureType::Text
    );
    SDL_FreeSurface(surface);
    return texture;
}

std::shared_ptr<Texture> Texture::fromColor(const glm::vec4& color)
{
    auto r = static_cast<uint8_t>(255.0f * color.x);
    auto g = static_cast<uint8_t>(255.0f * color.y);
    auto b = static_cast<uint8_t>(255.0f * color.z);
    return std::make_shared<Texture>(std::vector<uint8_t>{r, g, b, 255}, 1, 1, RGBA8, 1, TextureType::Diffuse);
}

glm::vec4 Texture::sample(float u, float v) const
{
    if (_data.empty()) return glm::vec4(0.0f);

    // Gestione coordinate (clamp to edge)
    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);

    float fx = u * (float)(_width - 1);
    float fy = v * (float)(_height - 1);

    int x0 = (int)std::floor(fx);
    int y0 = (int)std::floor(fy);
    int x1 = glm::min(x0 + 1, (int)_width - 1);
    int y1 = glm::min(y0 + 1, (int)_height - 1);

    float dx = fx - std::floor(fx);
    float dy = fy - std::floor(fy);

    auto getPixel = [&](int x, int y) -> glm::vec4 {
        size_t index = (y * _width + x) * _bytesPerPixel;
        if (index + _bytesPerPixel > _data.size()) return glm::vec4(0.0f);

        if (_format == RGBA32F || _format == R32F) {
            const float* p = reinterpret_cast<const float*>(&_data[index]);
            if (_format == RGBA32F) return glm::vec4(p[0], p[1], p[2], p[3]);
            else return glm::vec4(p[0], 0.0f, 0.0f, 1.0f);
        } else {
            const uint8_t* p = &_data[index];
            if (_format == RGBA8) return glm::vec4(p[0], p[1], p[2], p[3]) / 255.0f;
            else if (_format == RGB8) return glm::vec4(p[0], p[1], p[2], 255.0f) / 255.0f;
            else if (_format == R8) return glm::vec4(p[0], 0.0f, 0.0f, 255.0f) / 255.0f;
        }
        return glm::vec4(0.0f);
    };

    glm::vec4 p00 = getPixel(x0, y0);
    glm::vec4 p10 = getPixel(x1, y0);
    glm::vec4 p01 = getPixel(x0, y1);
    glm::vec4 p11 = getPixel(x1, y1);

    glm::vec4 p0 = glm::mix(p00, p10, dx);
    glm::vec4 p1 = glm::mix(p01, p11, dx);

    return glm::mix(p0, p1, dy);
}

[[nodiscard]] glm::vec4 Texture::sample(float u, float v, float w) const
{
    if (!_is3D || _depth == 0) return sample(u, v);

    // Gestione coordinate (clamp to edge)
    u = glm::clamp(u, 0.0f, 1.0f);
    v = glm::clamp(v, 0.0f, 1.0f);
    w = glm::clamp(w, 0.0f, 1.0f);

    float fx = u * (float)(_width - 1);
    float fy = v * (float)(_height - 1);
    float fz = w * (float)(_depth - 1);

    int x0 = (int)std::floor(fx);
    int y0 = (int)std::floor(fy);
    int z0 = (int)std::floor(fz);
    int x1 = glm::min(x0 + 1, (int)_width - 1);
    int y1 = glm::min(y0 + 1, (int)_height - 1);
    int z1 = glm::min(z0 + 1, (int)_depth - 1);

    float dx = fx - std::floor(fx);
    float dy = fy - std::floor(fy);
    float dz = fz - std::floor(fz);

    auto getPixel3D = [&](int x, int y, int z) -> glm::vec4 {
        size_t index = ((z * _height + y) * _width + x) * _bytesPerPixel;
        if (index + _bytesPerPixel > _data.size()) return glm::vec4(0.0f);

        if (_format == RGBA32F || _format == R32F) {
            const float* p = reinterpret_cast<const float*>(&_data[index]);
            if (_format == RGBA32F) return glm::vec4(p[0], p[1], p[2], p[3]);
            else return glm::vec4(p[0], 0.0f, 0.0f, 1.0f);
        } else {
            const uint8_t* p = &_data[index];
            if (_format == RGBA8) return glm::vec4(p[0], p[1], p[2], p[3]) / 255.0f;
            else if (_format == RGB8) return glm::vec4(p[0], p[1], p[2], 255.0f) / 255.0f;
            else if (_format == R8) return glm::vec4(p[0], 0.0f, 0.0f, 255.0f) / 255.0f;
        }
        return glm::vec4(0.0f);
    };

    glm::vec4 p000 = getPixel3D(x0, y0, z0);
    glm::vec4 p100 = getPixel3D(x1, y0, z0);
    glm::vec4 p010 = getPixel3D(x0, y1, z0);
    glm::vec4 p110 = getPixel3D(x1, y1, z0);
    glm::vec4 p001 = getPixel3D(x0, y0, z1);
    glm::vec4 p101 = getPixel3D(x1, y0, z1);
    glm::vec4 p011 = getPixel3D(x0, y1, z1);
    glm::vec4 p111 = getPixel3D(x1, y1, z1);

    glm::vec4 p00 = glm::mix(p000, p100, dx);
    glm::vec4 p10 = glm::mix(p010, p110, dx);
    glm::vec4 p0 = glm::mix(p00, p10, dy);

    glm::vec4 p01 = glm::mix(p001, p101, dx);
    glm::vec4 p11 = glm::mix(p011, p111, dx);
    glm::vec4 p1 = glm::mix(p01, p11, dy);
    return glm::mix(p0, p1, dz);
}
