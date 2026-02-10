//
// Created by Giovanni Bollati on 06/02/26.
//

#ifndef EVOLVING_PLANETS_ASSETMANAGER_HPP
#define EVOLVING_PLANETS_ASSETMANAGER_HPP

#include <filesystem>
#include <unordered_map>
#include <Mesh.hpp>
#include <IMeshLoader.hpp>
#include <unordered_set>

class AssetManager
{
public:
    explicit AssetManager(const std::shared_ptr<IMeshLoader>& meshLoader) : _meshLoader(meshLoader)
    {
        assert(_meshLoader && "mesh loader null during AssetManager construction");
    }
    std::shared_ptr<Mesh> getMesh(const std::string& path)
    {
        std::shared_ptr<Mesh> mesh;
        if (_failedMeshes.contains(path))
        {
            std::cout << "calling on failed" << std::endl;
            return nullptr;
        }
        if (!_meshes.contains(path))
        {
            mesh = _meshLoader->loadMesh(path);
            if (mesh == nullptr)
            {
                _failedMeshes.insert(path);
                return nullptr;
            }
            _meshes.insert({path, mesh});
        }
        else
        {
            if (_meshes.at(path).expired()) _meshes[path] = _meshLoader->loadMesh(path);
        }
        std::cout << "calling on valid" << std::endl;
        return _meshes.at(path).lock();
    }

    void clearExpired()
    {
        for (auto it = _meshes.begin(); it != _meshes.end();)
        {
            if (it->second.expired())
            {
                it = _meshes.erase(it);
            } else ++it;
        }
    }

    void invalidate(const std::string& path)
    {
        _meshes.erase(path);
        _failedMeshes.erase(path);
    }

private:
    std::shared_ptr<IMeshLoader> _meshLoader;
    std::unordered_map<std::string, std::weak_ptr<Mesh>> _meshes;
    std::unordered_set<std::string> _failedMeshes;
};

#endif //EVOLVING_PLANETS_ASSETMANAGER_HPP