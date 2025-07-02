//
// Created by Giovanni Bollati on 28/06/25.
//

#ifndef UIRENDERER_HPP
#define UIRENDERER_HPP

#include <Rendering/IRenderer.hpp>
#include <Rendering/UI/UIWindow.hpp>
#include <ranges>
#include <SDL_log.h>

#include "UILabel.hpp"

namespace Rendering::UI
{
    class UIRenderer
    {
    public:
        UIRenderer(
            IRenderer* renderer,
            const PSOConfig& psoConfig,
            const UIWindowStyle& style
        ) : _renderer(renderer), _psoConfig(psoConfig), _style(style), _renderableIDs({}), _currentDepth(0.0f) {};

        UIRenderer(UIRenderer&& other) noexcept = delete;
        UIRenderer& operator=(UIRenderer&& other) noexcept = delete;
        UIRenderer(const UIRenderer& other) = delete;
        UIRenderer& operator=(const UIRenderer& other) = delete;

        void update(UIElement* element)
        {
            if (element)
            {
                auto renderableIDs = element->submitToRenderer(this);
                _renderableIDs.insert(_renderableIDs.end(), renderableIDs.begin(), renderableIDs.end());
            }
            else
            {
                throw std::runtime_error("UIElement is null in UIRenderer::update");
            }
        }

        void clear()
        {
            //SDL_Log("Size of elements: %zu", _renderableIDs.size());

            for (const auto& id : _renderableIDs)
            {
                _renderer->removeRenderable(id);
            }
            _renderableIDs.clear();

            // reset depth
            _currentDepth = 0.0f;
        }

        std::vector<uint64_t> submitWindow(const UIWindow& window) const;
        std::vector<uint64_t> submitLabel(const UILabel& label) const;

    private:
        IRenderer* _renderer;
        const PSOConfig& _psoConfig;
        UIWindowStyle _style;
        std::vector<uint64_t> _renderableIDs;
        float _currentDepth; // Used to manage depth for UI elements
    };
}
#endif //UIRENDERER_HPP
