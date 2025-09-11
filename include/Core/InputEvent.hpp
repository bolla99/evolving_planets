//
// Created by Giovanni Bollati on 03/07/25.
//

#ifndef INPUTEVENT_HPP
#define INPUTEVENT_HPP

namespace Core
{
    enum class KeyCode : int {
        UNKNOWN = 0,

        A = 1, B = 2, C = 3, D = 4, E = 5, F = 6, G = 7, H = 8, I = 9, J = 10,
        K = 11, L = 12, M = 13, N = 14, O = 15, P = 16, Q = 17, R = 18, S = 19,
        T = 20, U = 21, V = 22, W = 23, X = 24, Y = 25, Z = 26,

        NUM_0 = 30, NUM_1 = 31, NUM_2 = 32, NUM_3 = 33, NUM_4 = 34,
        NUM_5 = 35, NUM_6 = 36, NUM_7 = 37, NUM_8 = 38, NUM_9 = 39,

        SPACE = 40,
        ENTER = 41,
        ESCAPE = 42,
        BACKSPACE = 43,
        TAB = 44,

        LEFT = 50,
        RIGHT = 51,
        UP = 52,
        DOWN = 53,

        SHIFT_LEFT = 60,
        SHIFT_RIGHT = 61,
        CTRL_LEFT = 62,
        CTRL_RIGHT = 63,
        ALT_LEFT = 64,
        ALT_RIGHT = 65,

        F1 = 70, F2 = 71, F3 = 72, F4 = 73, F5 = 74, F6 = 75,
        F7 = 76, F8 = 77, F9 = 78, F10 = 79, F11 = 80, F12 = 81
    };

    KeyCode sdlToKeyCode(int sdlKeyCode);

    enum class InputEventType
    {
        MouseMove,
        MouseLeftButtonDown,
        MouseLeftButtonUp,
        MouseRightButtonDown,
        MouseRightButtonUp,
        MouseMiddleButtonDown,
        MouseMiddleButtonUp,
        KeyDown,
        KeyUp
    };

    struct InputEvent
    {
        InputEventType type;
        int x;
        int y;
        KeyCode keyCode;
    };

}

#endif //INPUTEVENT_HPP
