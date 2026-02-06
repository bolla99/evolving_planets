//
// Created by Giovanni Bollati on 29/06/25.
//
#include <Rendering/UI/UIRenderer.hpp>

#include "glm/gtc/type_ptr.hpp"

namespace Rendering::UI
{
    std::vector<uint64_t> UIRenderer::submitWindow(const UIWindow& window) const
    {
        float depth = 0.02f;
        std::vector<uint64_t> renderableIDs;

        float posDragQuad[3] = {
            static_cast<float>(window.x() + window.width() - _style.dragAreaSize),
            static_cast<float>(window.y() + window.height() - _style.dragAreaSize),
            depth += 0.001f
        };

        auto dragQuad = Mesh::quad(
            posDragQuad,
            _style.dragAreaSize,
            _style.dragAreaSize,
            _style.dragAreaColor.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *dragQuad,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );

        float headerPos[3] = {
            static_cast<float>(window.x()),
            static_cast<float>(window.y()),
            depth += 0.001f
        };

        auto header = Mesh::quad(
            headerPos,
            window.width(),
            _style.headerSize,
            _style.headerColor.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *header,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );

        float borderDownPos[3] = {
            static_cast<float>(window.x()),
            static_cast<float>(window.y() + window.height() - _style.borderSize),
            depth += 0.001f
        };

        auto borderDown = Mesh::quad(
            borderDownPos,
            window.width(),
            _style.borderSize,
            _style.borderColor.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *borderDown,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );

        float borderLeftPos[3] = {
            static_cast<float>(window.x()),
            static_cast<float>(window.y()),
            depth += 0.001f
        };

        auto borderLeft = Mesh::quad(
            borderLeftPos,
            _style.borderSize,
            window.height(),
            _style.borderColor.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *borderLeft,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );

        float borderRightPos[3] = {
            static_cast<float>(window.x() + window.width() - _style.borderSize),
            static_cast<float>(window.y()),
            depth += 0.001f
        };

        auto borderRight = Mesh::quad(
            borderRightPos,
            _style.borderSize,
            window.height(),
            _style.borderColor.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *borderRight,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );

        float pos[3] = {
            static_cast<float>(window.x()),
            static_cast<float>(window.y()),
            depth += 0.001f
        };

        auto background = Mesh::quad(
            pos,
            static_cast<float>(window.width()),
            static_cast<float>(window.height()),
            _style.color.data(),
            1.0f, 1.0f
        );
        renderableIDs.push_back(
            _renderer->addRenderable(
                *background,
                Rendering::psoConfigs.at("UI"), {}, RenderLayer::UI
            )
        );
        return renderableIDs;
    }

    std::vector<uint64_t> UIRenderer::submitLabel(const UILabel& label) const
    {
        float pos[3] = {
            static_cast<float>(label.x()),
            static_cast<float>(label.y()),
            0.001f // Depth for the label
        };

        // create a quad that has height and width of the text
        // and position of the label, which is decided by the UIManager
        auto labelQuad = Mesh::quad(
            pos,
            static_cast<float>(label.width()),
            static_cast<float>(label.height()),
            glm::value_ptr(UILabel::defaultStyle().color),
            label.uvX(), label.uvY()
        );
        std::vector<uint64_t> renderableIDs;
        renderableIDs.push_back(
            _renderer->addRenderable(
                *labelQuad,
                Rendering::psoConfigs.at("TextureUI"),
                {label.texture()},
                RenderLayer::TEXT
            )
        );
        return renderableIDs;
    }
}
