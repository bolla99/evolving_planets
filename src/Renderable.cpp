//
// Created by Giovanni Bollati on 24/03/25.
//

#include <Renderable.hpp>
#include <PipelineStateObject.hpp>

Renderable::Renderable(
    const std::vector<std::pair<int, void*>>& data,
    const std::shared_ptr<PipelineStateObject>& pso,
    int verticesCount,
    MTL::Device* device
    )
{
    if (device == nullptr) throw std::runtime_error("Device is null");
    if (data.empty())
    {
        throw std::runtime_error("Data is empty");
    }
    if (pso == nullptr) throw std::runtime_error("PSO is null");

    // create buffers from data
    for (const auto & [fst, snd] : data)
    {
        auto buffer = NS::TransferPtr(device->newBuffer(snd, fst, MTL::ResourceStorageModeShared));
        _buffers.push_back(buffer);
    }

    _pso = pso;
    _verticesCount = verticesCount;
    _startIndex = 0;
    _modelMatrix = simd::float4x4(1.0f); // identity matrix
}

void Renderable::render(MTL::RenderCommandEncoder* render_command_encoder, const simd::float4x4& view_projection_matrix) const
{
    render_command_encoder->setRenderPipelineState(_pso->getMetalPSO());
    for (int i = 0; i < _buffers.size(); i++)
    {
        render_command_encoder->setVertexBuffer(_buffers[i].get(), 0, i);
    }
    auto mvp = view_projection_matrix * _modelMatrix;
    auto mvp_buffer = NS::TransferPtr(render_command_encoder->device()->newBuffer(&mvp, sizeof(mvp), MTL::ResourceStorageModeShared));
    render_command_encoder->setVertexBuffer(mvp_buffer.get(), 0, 30);

    render_command_encoder->drawPrimitives(
        MTL::PrimitiveType::PrimitiveTypeTriangle,
        static_cast<NS::UInteger>(_startIndex),
        static_cast<NS::UInteger>(_verticesCount));
};