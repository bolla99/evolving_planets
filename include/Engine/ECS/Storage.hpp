//
// Created by Giovanni Bollati on 29/01/26.
//

#ifndef EVOLVING_PLANETS_STORAGE_HPP
#define EVOLVING_PLANETS_STORAGE_HPP
#include <unordered_map>

#include "glm/fwd.hpp"


class IStorage
{
    public:
        virtual ~IStorage() = default;
        virtual void free(uint64_t id) = 0;
};

template <class T>
class Storage : public IStorage
{
public:
    Storage() = default;

    // does nothing id is is already added
    void add(uint64_t id, T data)
    {
        _data.insert({id, data});
    }

    [[nodiscard]] bool contains(uint64_t id) const
    {
        return _data.contains(id);
    }

    // throws exception if id is not found
    T& get(uint64_t id)
    {
        return _data.at(id);
    }

    // does nothing if id is not found
    void free(uint64_t id) override
    {
        _data.erase(id);
    }

    const std::unordered_map<uint64_t, T>& getAll() const
    {
        return _data;
    }

private:
    std::unordered_map<uint64_t, T> _data;
};

#endif //EVOLVING_PLANETS_STORAGE_HPP