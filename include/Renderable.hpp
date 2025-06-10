//
// Created by Giovanni Bollati on 13/03/25.
//

#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include <Metal/MTLRenderCommandEncoder.hpp>
#include <PipelineStateObject.hpp>
#include <simd/simd.h>

class Renderable
{
public:
    Renderable(
        const std::vector<std::pair<int, void*>>& data,
        const std::shared_ptr<PipelineStateObject>& pso,
        int verticesCount,
        MTL::Device* device
        );

    void render(MTL::RenderCommandEncoder* renderCommandEncoder, const simd::float4x4& viewProjectionMatrix) const;



private:
    std::vector<NS::SharedPtr<MTL::Buffer>> _buffers;
    std::shared_ptr<PipelineStateObject> _pso;
    int _verticesCount;
    int _startIndex;
    simd::float4x4 _modelMatrix;
};

#endif //RENDERABLE_HPP
