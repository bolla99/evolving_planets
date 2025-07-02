//
// Created by Giovanni Bollati on 25/06/25.
//

#ifndef UIELEMENT_HPP
#define UIELEMENT_HPP

#include <vector>

namespace Rendering::UI
{
    class UIRenderer;

    class UIElement
    {
    public:
        UIElement(int x, int y, int width, int height)
            : _x(x), _y(y), _width(width), _height(height) {}

        virtual ~UIElement() = default;

        UIElement(UIElement&& other) noexcept = default;
        UIElement& operator=(UIElement&& other) noexcept = default;
        UIElement(const UIElement& other) = default;
        UIElement& operator=(const UIElement& other) = default;

        [[nodiscard]] int x() const { return _x; }
        [[nodiscard]] int y() const { return _y; }
        [[nodiscard]] int width() const { return _width; }
        [[nodiscard]] int height() const { return _height; }

        void x(int value) { _x = value; }
        void y(int value) { _y = value; }
        void width(int value) { _width = value; }
        void height(int value) { _height = value; }

        /**
         * Registers the UI element with the specified UI renderer. This function is responsible
         * for integrating the UI element into the rendering pipeline, so it can be displayed
         * properly on the user interface.
         *
         * @param renderer A pointer to the UIRenderer object that will manage the rendering of the UI element.
         *                 The renderer must be a valid instance and handle the rendering logic.
         * @return A vector of 64-bit unsigned integers representing internal identifiers or
         *         handles used by the renderer to reference the registered UI elements.
         *         The exact contents and purpose of the returned identifiers are determined
         *         by the implementation details of the renderer.
         */
        virtual std::vector<uint64_t> submitToRenderer(UIRenderer* renderer) const = 0;

    protected:
        int _x, _y;
        int _width, _height;
    };
}

#endif //UIELEMENT_HPP
