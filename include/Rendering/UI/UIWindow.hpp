//
// Created by Giovanni Bollati on 25/06/25.
//

#ifndef UIWINDOW_HPP
#define UIWINDOW_HPP
#include <array>

#include "UIElement.hpp"
#include "Rendering/IRenderer.hpp"



namespace Rendering::UI
{
    // forward declaration for visitor pattern
    class UIRenderer;

    enum WindowState {
        Idle,
        Dragging,
        Resizing
    };

    struct UIWindowStyle
    {
        float headerSize;
        float borderSize;
        float dragAreaSize;
        float minSize;
        std::array<float, 4> color;
        std::array<float, 4> dragAreaColor;
        std::array<float, 4> headerColor;
        std::array<float, 4> hoverColor;
        std::array<float, 4> borderColor;

        static UIWindowStyle defaultStyle()
        {
            return {
                22.0f,
                5.0f,
                20.0f, // dragAreaSize
                100.0f, // minSize
                {0.2f, 0.2f, 0.3f, 1.0f}, // color
                {0.25f, 0.25f, 0.4f, 1.0f}, // dragAreaColor
                {0.25f, 0.25f, 0.4f, 1.0f}, // headerColor
            {0.2f, 0.2f, 0.5f, 1.0f},
            {0.25f, 0.25f, 0.4f, 1.0f},// hoverColor
            };
        }
    };

    class UIWindow : public UIElement
    {
    public:
        UIWindow(int x, int y, int width, int height)
            : UIElement(x, y, width, height), _state(Idle) {}

        [[nodiscard]] WindowState state() const { return _state; }
        void state(WindowState value) { _state = value; }

        std::vector<uint64_t> submitToRenderer(UIRenderer* renderer) const override;

        static UIWindowStyle style;


    private:
        WindowState _state;
    };
}

#endif //UIWINDOW_HPP
