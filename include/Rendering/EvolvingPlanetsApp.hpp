//
// Created by Giovanni Bollati on 29/01/26.
//

#ifndef EVOLVING_PLANETS_EVOLVINGPLANETSAPP_HPP
#define EVOLVING_PLANETS_EVOLVINGPLANETSAPP_HPP

#include "App.hpp"

class EvolvingPlanetsApp : public App
{
public:
    EvolvingPlanetsApp(int width, int height) : App(width, height) {};
    void init() override;
    void run() override;
};

#endif //EVOLVING_PLANETS_EVOLVINGPLANETSAPP_HPP