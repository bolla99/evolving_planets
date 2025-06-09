//
// Created by Giovanni Bollati on 09/06/25.
//

#ifndef PSOCONFIG_HPP
#define PSOCONFIG_HPP
#include <string>

#include "GlobalEnums.hpp"

struct PSOConfig
{
    std::string name;
    std::string vertexShader;
    std::string fragmentShader;
    VertexDescriptor vertexDescriptor;
};

extern const std::vector<PSOConfig> psoConfigs;

#endif //PSOCONFIG_HPP
