//
// Created by Giovanni Bollati on 09/02/26.
//

#ifndef EVOLVING_PLANETS_SYSTEMSMANAGER_HPP
#define EVOLVING_PLANETS_SYSTEMSMANAGER_HPP


#include <unordered_map>
#include "Systems.hpp"

class SystemsManager : public ISystem
{
public:
    SystemsManager() : _pool(), _systems() {}

    // add system of type T
    template <std::derived_from<ISystem> T>
    uint64_t addSystem()
    {
        auto id = _pool.newID();
        _systems.insert({id, std::make_shared<T>()});
        return id;
    }

    // remove every system of type T
    template <std::derived_from<ISystem> T>
    void removeSystem()
    {
        for (auto it = _systems.begin(); it != _systems.end();)
        {
            if (typeid(*it->second) == typeid(T))
            {
                _pool.destroyID(it->first);
                it = _systems.erase(it);
            } else
            {
                ++it;
            }
        }
    }
    // remove system with type id
    void removeSystem(const uint64_t id)
    {
        _pool.destroyID(id);
        _systems.erase(id);
    }

    void update(World& world, const Context& ctx, float dt) override
    {
        for (const auto& system : _systems | std::views::values) system->update(world, ctx, dt);
    }
private:
    Pool _pool;
    std::unordered_map<uint64_t, std::shared_ptr<ISystem>> _systems;
};

#endif //EVOLVING_PLANETS_SYSTEMSMANAGER_HPP