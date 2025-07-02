//
// Created by Giovanni Bollati on 28/06/25.
//

#include <Rendering/UI/UIWindow.hpp>

#include <Rendering/IRenderer.hpp>
#include <Rendering/PSOConfigs.hpp>

#include "Rendering/UI/UIRenderer.hpp"


namespace Rendering::UI
{
    std::vector<uint64_t> UIWindow::submitToRenderer(UIRenderer* renderer) const
    {
        return renderer->submitWindow(*this);
    }

    UIWindowStyle UIWindow::style = UIWindowStyle::defaultStyle();
}
