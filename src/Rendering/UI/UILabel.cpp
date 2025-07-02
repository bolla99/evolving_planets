//
// Created by Giovanni Bollati on 28/06/25.
//

#include "Rendering/UI/UILabel.hpp"

#include <vector>
#include <Rendering/UI/UIRenderer.hpp>

namespace Rendering::UI
{
    UILabel::UILabel(std::string text, int x, int y)
            : UIElement( x, y, 10, 10), _text(std::move(text)), _uvX(1.0f), _uvY(1.0f)
    {
        _texture = Texture::fromText(
            _text,
            static_cast<int>(defaultStyle().fontSize),
            defaultStyle().color
        );
        // set width and height accordingly to the texture size, which depends on the font size
        width(static_cast<int>(_texture->width()));
        height(static_cast<int>(_texture->height()));
    }

    std::vector<uint64_t> UILabel::submitToRenderer(UIRenderer* renderer) const
    {
        return renderer->submitLabel(*this);
    }
}

