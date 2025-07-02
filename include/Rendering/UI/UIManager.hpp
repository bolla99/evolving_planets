//
// Created by Giovanni Bollati on 25/06/25.
//

#ifndef UIMANAGER_HPP
#define UIMANAGER_HPP
#include <SDL_events.h>
#include <unordered_map>
#include <vector>
#include <string>

#include "UIWindow.hpp"

namespace Rendering::UI
{
    class WindowContext
    {
    public:
        int currentY = 0;
        int padding = 5;
        bool isActive = false;
        std::unique_ptr<UIWindow> window;
        std::vector<std::unique_ptr<UIElement>> elements;

        WindowContext(int x, int y, int width, int height)
            : window(std::make_unique<UIWindow>(x, y, width, height)) {}

        void resetLayout()
        {
            currentY = window->style.headerSize + window->y() + padding;
        }

        [[nodiscard]] int nextY(int elementHeight) const
        {
            return currentY + elementHeight + padding;
        }
    };

    class UIManager
    {
    public:
        explicit UIManager(
            std::unique_ptr<UIRenderer> uiRenderer
            ) : _uiRenderer(std::move(uiRenderer)) {}

        bool processInput(SDL_Event event);
        void update();
        void clear();

        void beginWindow(const std::string& name);
        void endWindow();
        void text(const std::string& text);

    private:
        std::unique_ptr<UIRenderer> _uiRenderer;

        std::unordered_map<std::string, std::unique_ptr<WindowContext>> _contexts;
        WindowContext* _currentContext;
    };
}

#endif //UIMANAGER_HPP
