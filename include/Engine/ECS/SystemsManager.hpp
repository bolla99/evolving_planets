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
        _order.push_back(id);
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
                // destroy id
                _pool.destroyID(it->first);
                // remove from system map
                it = _systems.erase(it);
                // remove from order
                _order.erase(std::ranges::find(_order, it->first));
            }
            else
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
        _order.erase(std::ranges::find(_order, id));
    }

    void update(World& world, const Context& ctx, float dt) override
    {
        for (auto& id : _order)
        {
            _systems[id]->update(world, ctx, dt);
        }
    }

    std::string name() const override { return "SystemsManager"; }
private:
    Pool _pool;
    std::unordered_map<uint64_t, std::shared_ptr<ISystem>> _systems;
    std::vector<uint64_t> _order;
};

#endif //EVOLVING_PLANETS_SYSTEMSMANAGER_HPP