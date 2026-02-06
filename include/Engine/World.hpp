//
// Created by Giovanni Bollati on 03/02/26.
//

#ifndef EVOLVING_PLANETS_WORLD_HPP
#define EVOLVING_PLANETS_WORLD_HPP

#include <typeindex>
#include "Pool.hpp"
#include "Storage.hpp"
#include <unordered_map>
#include <memory>
#include <ranges>
#include <vector>
#include <functional>
#include <typeinfo>

#include "Components.hpp"


class World
{
public:
    World() = default;

    uint64_t createEntity(const std::string& name)
    {
        auto id = _entities.newID();
        addComponent<NameComponent>(id, {name});
        return id;
    }

    void destroyEntity(uint64_t entityID)
    {
        _entities.destroyID(entityID);

        for (auto& storage : _storages | std::views::values)
        {
            storage->free(entityID);
        }
    }

    const std::vector<uint64_t>& getEntities()
    {
        return _entities.getAll();
    }

    [[nodiscard]] bool hasEntity(uint64_t entityID) const
    {
        return _entities.isAlive(entityID);
    }

    template <typename T>
    bool hasComponent(uint64_t entityID)
    {
        return getOrCreateStorage<T>().contains(entityID);
    }
    template <typename T>
    T& getComponent(uint64_t entityID)
    {
        if (hasComponent<T>(entityID))
            return getOrCreateStorage<T>().get(entityID);

        throw std::runtime_error("Entity " + std::to_string(entityID) + " does not have " + typeid(T).name() + "component");
    }

    template <typename T>
    void addComponent(uint64_t entityID, T component)
    {
        getOrCreateStorage<T>().add(entityID, component);
    }

    template <typename T>
    void removeComponent(uint64_t entityID)
    {
        getOrCreateStorage<T>().free(entityID);
    }

    template <typename... Components>
    std::vector<uint64_t> query() {
        std::vector<uint64_t> matches;
        for (auto id : _entities.getAll()) {
            if ((hasComponent<Components>(id) && ...)) {
                matches.push_back(id);
            }
        }
        return matches;
    }

    void entityInspector(uint64_t entity)
    {
        if (hasComponent<Transform>(entity))
        {
            DrawComponentUI(getComponent<Transform>(entity));
        }
        if (hasComponent<DirectionalLightComponent>(entity))
        {
            DrawComponentUI(getComponent<DirectionalLightComponent>(entity));
        }
        if (hasComponent<PointLightComponent>(entity))
        {
            DrawComponentUI(getComponent<PointLightComponent>(entity));
        }
    }

private:
    Pool _entities;
    std::unordered_map<std::type_index, std::unique_ptr<IStorage>> _storages;

    template<class T>
    Storage<T>& getOrCreateStorage()
    {
        auto type = std::type_index(typeid(T));
        if (!_storages.contains(type))
        {
            _storages.insert({type, std::make_unique<Storage<T>>()});
        }
        return *static_cast<Storage<T>*>(_storages[type].get());
    }
};

#endif //EVOLVING_PLANETS_WORLD_HPP