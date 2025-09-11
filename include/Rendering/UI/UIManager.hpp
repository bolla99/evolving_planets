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
            ) :
        _uiRenderer(std::move(uiRenderer)),
        _currentContext(nullptr) {}

        /**
         * Process an SDL event.
         * @param event SDL event to process
         * @return true if the event was processed by the UIManager, false otherwise.
         */
        bool processInput(SDL_Event event);

        /**
         * To be called after the last endWindow call.
         */
        void update();

        /**
         * To be called before the first beginWindow call.
         */
        void clear();

        void beginWindow(const std::string& name);
        void endWindow();

        // elements
        void text(const std::string& text);
        bool button(const std::string& text);

    private:
        // renderer
        std::unique_ptr<UIRenderer> _uiRenderer;
        // window contexts
        std::unordered_map<std::string, std::unique_ptr<WindowContext>> _contexts;
        // current context
        WindowContext* _currentContext;
    };
}

#endif //UIMANAGER_HPP
