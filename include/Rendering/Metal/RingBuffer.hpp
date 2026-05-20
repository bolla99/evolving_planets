//
// Created by Giovanni Bollati on 16/02/26.
//

#ifndef EVOLVING_PLANETS_RINGBUFFER_HPP
#define EVOLVING_PLANETS_RINGBUFFER_HPP

#include <iostream>
#include <ostream>
#include <Metal/Metal.hpp>

class RingBuffer
{
public:
    RingBuffer(MTL::Device* device, std::size_t maxSize = 10000000, size_t alignment = 256)
    {
        auto alignedSize = ((maxSize) / alignment) * alignment;
        framesInFlight = 3;
        frameBlockSizeBytes = ((alignedSize / framesInFlight) / alignment) * alignment;
        capacity = frameBlockSizeBytes * framesInFlight;
        buffer = NS::TransferPtr(
            device->newBuffer(capacity, MTL::ResourceStorageModeShared)
            );
        currentFrame = 0;
        head = 0;
        this->alignment = alignment;
    }

    size_t write(const std::vector<std::byte>& data)
    {
        // copy data
        if (head % frameBlockSizeBytes + data.size() > frameBlockSizeBytes)
        {
            std::cerr << "RingBuffer overflow" << std::endl;
            throw std::runtime_error("RingBuffer overflow");
        }
        if (head + data.size() >= capacity)
        {
            std::cerr << "RingBuffer overflow" << std::endl;
            throw std::runtime_error("RingBuffer overflow");
        }

        memcpy(static_cast<uint8_t*>(buffer->contents()) + head, data.data(), data.size());
        size_t offset = head;
        // update head
        if (data.size() < alignment)
        {
            head += alignment;
        }
        else
        {
            auto factor = data.size() / alignment;
            head += (factor + (data.size() % alignment != 0 ? 1 : 0)) * alignment;
        }
        return offset;
    }

    void beginFrame()
    {
        currentFrame = (currentFrame + 1) % framesInFlight;
        head = currentFrame * frameBlockSizeBytes;
    }

    NS::SharedPtr<MTL::Buffer> buffer;
    size_t capacity;
    size_t frameBlockSizeBytes;
    int framesInFlight;
    int currentFrame;
    size_t head;
    size_t alignment;

};

#endif //EVOLVING_PLANETS_RINGBUFFER_HPP