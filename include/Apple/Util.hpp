//
// Created by Giovanni Bollati on 22/06/25.
//

#ifndef UTIL_HPP
#define UTIL_HPP

#include <Foundation/Foundation.hpp>>
#include <string>

namespace Apple
{
    inline std::string resourcePath(std::string&& resourceName)
    {
        auto bundleResourcePath = NS::Bundle::mainBundle()->resourcePath();
        if (bundleResourcePath == nullptr) {
            throw std::runtime_error("Failed to get resource path from main bundle");
        }
        return std::string(bundleResourcePath->utf8String()) + "/" + resourceName;
    }
}

#endif //UTIL_HPP
