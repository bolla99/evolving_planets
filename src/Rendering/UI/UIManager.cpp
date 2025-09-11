//
// Created by Giovanni Bollati on 25/06/25.
//


#include "Rendering/UI/UIManager.hpp"

#include <vector>
#include <ranges>

#include "Rendering/IRenderer.hpp"
#include "Rendering/UI/UIWindow.hpp"
#include <Rendering/PSOConfigs.hpp>
#include "Rendering/UI/UIRenderer.hpp"


namespace Rendering::UI
{
    bool UIManager::processInput(SDL_Event event)
    {
        if (event.type == SDL_MOUSEBUTTONUP)
        {
            // Reset dragged state when mouse button is released
            for (auto& context : _contexts | std::views::values)
            {
                context->window->state(Idle); // Reset window state to Idle
            }
            return true; // Event processed
        }
        for (auto& context : _contexts | std::views::values)
        {
            auto windowData = context->window.get();

            // Process input for each window
            // This is a placeholder, actual input processing logic will depend on the window's requirements
            // For example, you might want to check if the event is a mouse click within the window bounds
            // and handle it accordingly.
            if (event.type == SDL_MOUSEBUTTONDOWN)
            {
                // RESIZE
                if (event.button.x >= windowData->x() + windowData->width() - UIWindow::style.dragAreaSize &&
                    // Check if click is near the right edge)
                    event.button.y >= windowData->y() + windowData->height() - UIWindow::style.dragAreaSize &&
                    // and bottom edge
                    event.button.x <= windowData->x() + windowData->width() &&
                    event.button.y <= windowData->y() + windowData->height())
                {
                    // Handle resize action
                    windowData->state(Resizing); // Mark the window as resizing
                    return true;
                }

                // DRAG
                if (event.button.x >= windowData->x() && event.button.x <= windowData->x() + windowData->width() &&
                    event.button.y >= windowData->y() && event.button.y <= windowData->y() + windowData->height())
                {
                    // Handle mouse click within the window
                    // For example, you could change the window's color or trigger some action
                    windowData->state(Dragging); // Mark the window as dragged
                    // move the window to the front
                    // This ensures that the window is always on top of the others
                    return true;
                }
            }
            else if (
                event.type == SDL_MOUSEMOTION
                && event.motion.state & SDL_BUTTON(SDL_BUTTON_LEFT) // Check if left mouse button is pressed
            )
            {
                if (windowData->state() == Resizing)
                {
                    // RESIZE WINDOW -> MODIFY WIDTH AND HEIGHT
                    windowData->width(windowData->width() + event.motion.xrel); // Resize window horizontally
                    windowData->height(windowData->height() + event.motion.yrel); // Resize window vertically
                    return true;
                }
                if (windowData->state() == Dragging)
                {
                    // DRAG WINDOW -> MODIFY X AND Y
                    windowData->x(windowData->x() + event.motion.xrel); // Move window horizontally
                    windowData->y(windowData->y() + event.motion.yrel); // Move window vertically
                    return true;
                }
            }
        }
        return false;
    }

    void UIManager::update()
    {
        for (auto& context : _contexts)
        {
            _uiRenderer->update(context.second->window.get());
            for (const auto& element : context.second->elements)
            {
                _uiRenderer->update(element.get());
            }

        }
    }

    void UIManager::clear()
    {
        _uiRenderer->clear();
        for (auto& context : _contexts)
        {
            context.second->resetLayout();
            context.second->elements.clear();
        }
        _currentContext = nullptr;
    }

    void UIManager::beginWindow(const std::string& name)
    {
        if (!_contexts.contains(name))
        {
            SDL_Log("Creating new window context for: %s", name.c_str());
            // create a WindowContext
            auto windowContext = std::make_unique<WindowContext>(100, 100, 200, 200);
            // Reset layout for the new window context
            windowContext->resetLayout();
            // Create a UIWindow and add it to the context
            _contexts.emplace(name, std::move(windowContext));
            _currentContext = _contexts[name].get();
        }
        else
        {
            _currentContext = _contexts[name].get();
        }

        auto headerLabel = std::make_unique<UILabel>(
                name,
                _currentContext->window->x() + _currentContext->padding + _currentContext->window->style.borderSize,
                _currentContext->window->y()
            );
        int drawableWidth = _currentContext->window->width() - 2 * (_currentContext->padding + _currentContext->window->style.borderSize);
        headerLabel->uvX(std::min(1.0f, drawableWidth / static_cast<float>(headerLabel->width())));
        headerLabel->width(headerLabel->width() * headerLabel->uvX());
        _currentContext->elements.push_back(std::move(headerLabel));
    }

    void UIManager::endWindow()
    {
        if (!_currentContext)
        {
            throw std::runtime_error("No current window context to end.");
        }
        _currentContext = nullptr;
    }

    void UIManager::text(const std::string& text)
    {
        if (_currentContext)
        {
            int x = _currentContext->window->x() + _currentContext->padding + _currentContext->window->style.borderSize;
            auto label = std::make_unique<UILabel>(text, x, _currentContext->currentY);
            // Update the current Y position for the next element
            _currentContext->currentY = _currentContext->nextY(label->height());
            int drawableWidth = _currentContext->window->width() - 2 * (_currentContext->padding + _currentContext->window->style.borderSize);
            label->uvX(std::min(1.0f, drawableWidth / static_cast<float>(label->width())));
            label->width(label->width() * label->uvX());
            _currentContext->elements.push_back(std::move(label));
        }
    }

    bool UIManager::button(const std::string& text)
    {
        return true;
    }
}
