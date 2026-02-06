//
// Created by Giovanni Bollati on 29/01/26.
//

#ifndef EVOLVING_PLANETS_POOL_HPP
#define EVOLVING_PLANETS_POOL_HPP

#include <vector>
#include <ranges>
#include <algorithm>
#include <functional>

class Pool
{
public:

    Pool() : _nextID(1) {}

    uint64_t newID()
    {
        uint64_t newID;
        if (!_freeIDs.empty())
        {
            newID = _freeIDs.back();
            _freeIDs.pop_back();
        } else
        {
            newID = _nextID++;
        }
        _aliveIDs.push_back(newID);
        return newID;
    }
    void destroyID(uint64_t id)
    {
        auto pos = std::ranges::find(_aliveIDs, id);
        if (pos == _aliveIDs.end()) return;
        *pos = _aliveIDs.back();
        _aliveIDs.pop_back();
        _freeIDs.push_back(id);
    }
    const std::vector<uint64_t>& getAll()
    {
        return _aliveIDs;
    }

    [[nodiscard]] bool isAlive(uint64_t id) const
    {
        return std::ranges::find(_aliveIDs, id) != _aliveIDs.end();
    }

private:
    uint64_t _nextID;
    std::vector<uint64_t> _freeIDs;
    std::vector<uint64_t> _aliveIDs;
};

#endif //EVOLVING_PLANETS_POOL_HPP
