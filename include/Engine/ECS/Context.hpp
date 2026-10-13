//
// Created by Giovanni Bollati on 01/07/26.
//

#ifndef EVOLVING_PLANETS_CONTEXT_HPP
#define EVOLVING_PLANETS_CONTEXT_HPP


#include <SDL_video.h>

#include <utility>

#include "AssetManager.hpp"
#include "Engine/Input.hpp"
#include "Rendering/IRenderer.hpp"

struct Context
{
    Rendering::IRenderer* renderer;
    SDL_Window* window;
    AssetManager* assetManager;
    Core::Input* input = nullptr;

    [[nodiscard]] bool isValid() const
    {
        return renderer != nullptr && window != nullptr && assetManager != nullptr;
    }
};

struct ScriptComponent {
    using ScriptFunc = std::function<void(World&, const Context&, uint64_t, float)>;
    ScriptComponent() = default;
    ScriptComponent(ScriptFunc&  beforeScriptFunc, ScriptFunc&  afterScriptFunc) : beforeUpdate(beforeScriptFunc), afterUpdate(afterScriptFunc) {}
    static ScriptComponent before(ScriptFunc& beforeScriptFunc)
    {
        auto sc = ScriptComponent();
        sc.beforeUpdate = beforeScriptFunc;
        return sc;
    }
    static ScriptComponent after(ScriptFunc& afterScriptFunc)
    {
        auto sc = ScriptComponent();
        sc.afterUpdate = afterScriptFunc;
        return sc;
    }
    ScriptFunc beforeUpdate = [](World&, const Context&, uint64_t, float) {};
    ScriptFunc afterUpdate = [](World&, const Context&, uint64_t, float) {};
};


#endif //EVOLVING_PLANETS_CONTEXT_HPP
