//
// Created by Giovanni Bollati on 26/06/25.
//

#include <Texture.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>
#include <string>
#include <SDL_ttf.h>
#include <simd/packed.h>

#include "Apple/Util.hpp"

Texture::Texture(
        const std::vector<uint8_t>& data,
        size_t width,
        size_t height,
        PixelFormat format,
        size_t bytesPerPixel
        ) :   _data(data),
              _width(width),
              _height(height),
              _format(format),
              _bytesPerPixel(bytesPerPixel) {}


std::shared_ptr<Texture> Texture::fromFile(const std::string& filePath)
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
            channels
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
        4
    );
    SDL_FreeSurface(surface);
    return texture;
}
