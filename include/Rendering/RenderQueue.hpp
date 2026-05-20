//
// Created by Giovanni Bollati on 10/02/26.
//

#ifndef EVOLVING_PLANETS_RENDERQUEUE_HPP
#define EVOLVING_PLANETS_RENDERQUEUE_HPP


#include <ranges>

#include "RenderLayers.hpp"
#include <unordered_map>
#include "RenderItem.hpp"

struct RenderQueue
{
    RenderQueue() = default;

    std::array<std::unordered_map<std::string, std::vector<RenderItem>>, 5> queue;

    RenderQueue& add(const RenderItem& item, const std::string& psoName, Rendering::RenderLayer layer)
    {
        auto ilayer = static_cast<int>(layer);
        if (!queue[ilayer].contains(psoName)) queue[ilayer].insert({psoName, {}});
        queue[ilayer][psoName].push_back(item);
        return *this;
    }
    void clear()
    {
        for (auto& map : queue)
        {
            map.clear();
        }
    }
};

#endif //EVOLVING_PLANETS_RENDERQUEUE_HPP