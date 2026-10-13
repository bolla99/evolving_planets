//
// Created by Giovanni Bollati on 01/07/26.
//

#ifndef EVOLVING_PLANETS_INPUT_HPP
#define EVOLVING_PLANETS_INPUT_HPP

#include <SDL_scancode.h>
#include <SDL_keyboard.h>
#include <cstring>
#include <algorithm>

namespace Core
{
    class Input
    {
    public:
        Input()
        {
            std::memset(_currentKeyboardState, 0, SDL_NUM_SCANCODES);
            std::memset(_previousKeyboardState, 0, SDL_NUM_SCANCODES);
        }

        bool isCursorActive = true;
        int mouseXRel = 0;
        int mouseYRel = 0;

        void update()
        {
            SDL_GetRelativeMouseState(&mouseXRel, &mouseYRel);

            // Copy current state to previous state
            std::memcpy(_previousKeyboardState, _currentKeyboardState, SDL_NUM_SCANCODES);

            // Get the new live state from SDL
            int numKeys = 0;
            const uint8_t* state = SDL_GetKeyboardState(&numKeys);

            int keysToCopy = std::min(numKeys, static_cast<int>(SDL_NUM_SCANCODES));
            if (keysToCopy > 0 && state != nullptr)
            {
                std::memcpy(_currentKeyboardState, state, keysToCopy);

                // If the state returned less than the maximum scancodes, pad the rest with 0
                if (keysToCopy < SDL_NUM_SCANCODES)
                {
                    std::memset(_currentKeyboardState + keysToCopy, 0, SDL_NUM_SCANCODES - keysToCopy);
                }
            }
        }

        [[nodiscard]] bool isPressed(SDL_Scancode scancode) const
        {
            if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) return false;
            return _currentKeyboardState[scancode] != 0;
        }

        [[nodiscard]] bool isJustPressed(SDL_Scancode scancode) const
        {
            if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) return false;
            return (_currentKeyboardState[scancode] != 0) && (_previousKeyboardState[scancode] == 0);
        }

        [[nodiscard]] bool isJustReleased(SDL_Scancode scancode) const
        {
            if (scancode < 0 || scancode >= SDL_NUM_SCANCODES) return false;
            return (_currentKeyboardState[scancode] == 0) && (_previousKeyboardState[scancode] != 0);
        }

    private:
        uint8_t _currentKeyboardState[SDL_NUM_SCANCODES];
        uint8_t _previousKeyboardState[SDL_NUM_SCANCODES];
    };
}

#endif //EVOLVING_PLANETS_INPUT_HPP
