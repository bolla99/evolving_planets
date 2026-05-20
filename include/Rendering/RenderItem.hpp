//
// Created by Giovanni Bollati on 10/02/26.
//

#ifndef EVOLVING_PLANETS_RENDERITEM_HPP
#define EVOLVING_PLANETS_RENDERITEM_HPP

#include <string>
#include <cstdint>
#include <vector>
#include "Material.hpp"
#include <stdexcept>
#include <unordered_map>

struct RenderItem
{
    RenderItem() = delete;
    explicit RenderItem(
        uint64_t renderableID,
        uint64_t materialID,
        bool castShadows = false,
        bool wireframe = false,
        glm::ivec3 grid = {0, 0, 0},
        glm::ivec3 threadgroup = {0, 0, 0}
        ) :
    rID(renderableID),
    mID(materialID),
    castShadows(castShadows),
    wireframe(wireframe),
    gridSize(grid.x, grid.y, grid.z),
    threadgroupSize(threadgroup.x, threadgroup.y, threadgroup.z)
    {}

    struct DispatchSize { uint32_t x, y, z; };

    // members
    uint64_t rID;
    uint64_t mID;
    bool castShadows;
    bool wireframe;
    DispatchSize gridSize;
    DispatchSize threadgroupSize;

};

#endif //EVOLVING_PLANETS_RENDERITEM_HPP