//
// Created by Giovanni Bollati on 13/02/26.
//

#ifndef EVOLVING_PLANETS_RENDERCONFIG_HPP
#define EVOLVING_PLANETS_RENDERCONFIG_HPP

#include <unordered_map>
#include <string>

enum FillMode
{
    Solid,
    Wireframe
};

struct RenderConfig
{
    FillMode fillMode = FillMode::Solid;
};

const std::unordered_map<std::string, const RenderConfig> renderConfigs {
    {
        "Default",
        {
            FillMode::Solid
        }
    }
};




#endif //EVOLVING_PLANETS_RENDERCONFIG_HPP