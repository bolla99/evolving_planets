//
// Created by Giovanni Bollati on 29/06/25.
//
#include <Rendering/UI/UIRenderer.hpp>
#include <Mesh.hpp>
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
            glm::vec4(posDragQuad[0], posDragQuad[1], _style.dragAreaSize, _style.dragAreaSize),
            posDragQuad[2],
            glm::vec4(_style.dragAreaColor[0], _style.dragAreaColor[1], _style.dragAreaColor[2], 1.0f),
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
            glm::vec4(headerPos[0], headerPos[1], static_cast<float>(window.width()), _style.headerSize),
            headerPos[2],
            glm::vec4(_style.headerColor[0], _style.headerColor[1], _style.headerColor[2], 1.0f),
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
            glm::vec4(borderDownPos[0], borderDownPos[1], static_cast<float>(window.width()), _style.borderSize),
            borderDownPos[2],
            glm::vec4(_style.borderColor[0], _style.borderColor[1], _style.borderColor[2], 1.0f),
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
            glm::vec4(borderLeftPos[0], borderLeftPos[1], _style.borderSize, static_cast<float>(window.height())),
            borderLeftPos[2],
            glm::vec4(_style.borderColor[0], _style.borderColor[1], _style.borderColor[2], 1.0f),
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
            glm::vec4(borderRightPos[0], borderRightPos[1], _style.borderSize, static_cast<float>(window.height())),
            borderRightPos[2],
            glm::vec4(_style.borderColor[0], _style.borderColor[1], _style.borderColor[2], 1.0f),
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
            glm::vec4(pos[0], pos[1], static_cast<float>(window.width()), static_cast<float>(window.height())),
            pos[2],
            glm::vec4(_style.color[0], _style.color[1], _style.color[2], 1.0f),
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
        // Position is in pixels (top-left origin) and depth is used to order.
        const float depth = 0.001f;

        auto labelQuad = Mesh::quad(
            glm::vec4(static_cast<float>(label.x()), static_cast<float>(label.y()), static_cast<float>(label.width()), static_cast<float>(label.height())),
            depth,
            UILabel::defaultStyle().color,
            label.uvX(),
            label.uvY()
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
