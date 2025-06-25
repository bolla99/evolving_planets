//
// Created by Giovanni Bollati on 12/06/25.
//

#ifndef ICOMMANDENCODER_HPP
#define ICOMMANDENCODER_HPP

#include "IPSO.hpp"

namespace Rendering
{
    class ICommandEncoder
    {
    public:
        ICommandEncoder() = default;
        // virtual destructor for polymorphism
        virtual ~ICommandEncoder() = default;
        // delete copy
        ICommandEncoder(const ICommandEncoder&) = delete;
        ICommandEncoder& operator=(const ICommandEncoder&) = delete;
        // delete move
        ICommandEncoder(ICommandEncoder&&) = delete;
        ICommandEncoder& operator=(ICommandEncoder&&) = delete;

        // bind the pso
        virtual void bind(IPSO* pso) = 0;

        // get the raw pointer to the api data structure
        virtual void* raw() = 0;
    };
}

#endif //ICOMMANDENCODER_HPP
