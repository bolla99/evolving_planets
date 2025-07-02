//
// Created by Giovanni Bollati on 25/06/25.
//

#ifndef UILABEL_HPP
#define UILABEL_HPP
#include "glm/vec4.hpp"
#include <string>
#include <array>
#include <Texture.hpp>

#include "UIElement.hpp"

namespace Rendering::UI
{
    // forward declaration
    class UIRenderer;

    struct UILabelStyle
    {
        glm::vec4 color;
        float fontSize;

        static UILabelStyle defaultStyle()
        {
            return {
                glm::vec4{1.0f, 1.0f, 1.0f, 0.2f}, // white
                16.0f // fontSize
            };
        }
    };

    class UILabel : public UIElement
    {
    public:
        UILabel(std::string text, int x, int y);

        std::vector<uint64_t> submitToRenderer(UIRenderer* renderer) const override;

        [[nodiscard]] const std::string& text() const { return _text; }

        static UILabelStyle defaultStyle()
        {
            return UILabelStyle::defaultStyle();
        }

        [[nodiscard]] float uvX() const { return _uvX; }
        [[nodiscard]] float uvY() const { return _uvY; }

        void uvX(float value) { _uvX = value; }
        void uvY(float value) { _uvY = value; }

        [[nodiscard]] std::shared_ptr<Texture> texture() const { return _texture; }

    private:
        std::string _text;
        float _uvX; // UV coordinates for texture mapping
        float _uvY; // UV coordinates for texture mapping
        std::shared_ptr<Texture> _texture;
    };
}

#endif //UILABEL_HPP
