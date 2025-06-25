//
// Created by Giovanni Bollati on 11/06/25.
//

#ifndef IMESHLOADER_HPP
#define IMESHLOADER_HPP

#include "Mesh.hpp"

class IMeshLoader
{
public:
    virtual ~IMeshLoader() = default;
    [[nodiscard]] virtual std::shared_ptr<Mesh> loadMesh(
        const std::string& path) const = 0;
};

#endif //IMESHLOADER_HPP
