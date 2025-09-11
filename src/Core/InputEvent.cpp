//
// Created by Giovanni Bollati on 03/07/25.
//

#include "Core/InputEvent.hpp"
#include <SDL_keycode.h>

namespace Core
{
    KeyCode sdlToKeyCode(int sdlKeyCode)
    {
        switch (sdlKeyCode)
        {
        case SDLK_a: return KeyCode::A;
        case SDLK_b: return KeyCode::B;
        case SDLK_c: return KeyCode::C;
        case SDLK_d: return KeyCode::D;
        case SDLK_e: return KeyCode::E;
        case SDLK_f: return KeyCode::F;
        case SDLK_g: return KeyCode::G;
        case SDLK_h: return KeyCode::H;
        case SDLK_i: return KeyCode::I;
        case SDLK_j: return KeyCode::J;
        case SDLK_k: return KeyCode::K;
        case SDLK_l: return KeyCode::L;
        case SDLK_m: return KeyCode::M;
        case SDLK_n: return KeyCode::N;
        case SDLK_o: return KeyCode::O;
        case SDLK_p: return KeyCode::P;
        case SDLK_q: return KeyCode::Q;
        case SDLK_r: return KeyCode::R;
        case SDLK_s: return KeyCode::S;
        case SDLK_t: return KeyCode::T;
        case SDLK_u: return KeyCode::U;
        case SDLK_v: return KeyCode::V;
        case SDLK_w: return KeyCode::W;
        case SDLK_x: return KeyCode::X;
        case SDLK_y: return KeyCode::Y;
        case SDLK_z: return KeyCode::Z;
        case SDLK_0: return KeyCode::NUM_0;
        case SDLK_1: return KeyCode::NUM_1;
        case SDLK_2: return KeyCode::NUM_2;
        case SDLK_3: return KeyCode::NUM_3;
        case SDLK_4: return KeyCode::NUM_4;
        case SDLK_5: return KeyCode::NUM_5;
        case SDLK_6: return KeyCode::NUM_6;
        case SDLK_7: return KeyCode::NUM_7;
        case SDLK_8: return KeyCode::NUM_8;
        case SDLK_9: return KeyCode::NUM_9;
        case SDLK_SPACE: return KeyCode::SPACE;
        case SDLK_ESCAPE: return KeyCode::ESCAPE;
        case SDLK_RETURN: return KeyCode::ENTER;
        case SDLK_TAB: return KeyCode::TAB;
        case SDLK_BACKSPACE: return KeyCode::BACKSPACE;
        case SDLK_UP: return KeyCode::UP;
        case SDLK_DOWN: return KeyCode::DOWN;
        case SDLK_LEFT: return KeyCode::LEFT;
        case SDLK_RIGHT: return KeyCode::RIGHT;
        case SDLK_F1: return KeyCode::F1;
        case SDLK_F2: return KeyCode::F2;
        case SDLK_F3: return KeyCode::F3;
        case SDLK_F4: return KeyCode::F4;
        case SDLK_F5: return KeyCode::F5;
        case SDLK_F6: return KeyCode::F6;
        case SDLK_F7: return KeyCode::F7;
        case SDLK_F8: return KeyCode::F8;
        case SDLK_F9: return KeyCode::F9;
        case SDLK_F10: return KeyCode::F10;
        case SDLK_F11: return KeyCode::F11;
        case SDLK_F12: return KeyCode::F12;
        case SDLK_LCTRL: return KeyCode::CTRL_LEFT;
        case SDLK_RCTRL: return KeyCode::CTRL_RIGHT;
        case SDLK_LALT: return KeyCode::ALT_LEFT;
        case SDLK_RALT: return KeyCode::ALT_RIGHT;
        case SDLK_LSHIFT: return KeyCode::SHIFT_LEFT;
        case SDLK_RSHIFT: return KeyCode::SHIFT_RIGHT;

        default: return KeyCode::UNKNOWN;
        }
    }
}
