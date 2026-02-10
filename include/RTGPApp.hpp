//
// Created by Giovanni Bollati on 29/01/26.
//

#ifndef EVOLVING_PLANETS_RTGPAPP_HPP
#define EVOLVING_PLANETS_RTGPAPP_HPP

#include "App.hpp"
#include "AssetManager.hpp"
#include <AssimpMeshLoader.hpp>

class RTGPApp : public App
{
public:
    RTGPApp(int width, int height) : _assetManager(AssetManager(std::make_shared<AssimpMeshLoader>())), App(width, height) {};
    void init() override;
    void run() override;

private:
    AssetManager _assetManager;
};


#endif //EVOLVING_PLANETS_RTGPAPP_HPP