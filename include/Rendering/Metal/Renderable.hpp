//
// Created by Giovanni Bollati on 13/03/25.
//

#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include "Metal/MTLRenderCommandEncoder.hpp"
#include <simd/simd.h>
#include "PSO.hpp"
#include "../ICommandEncoder.hpp"
#include "../IRenderable.hpp"
#include <Texture.hpp>

namespace Rendering::Metal
{
    class Renderable : public IRenderable
    {
    public:
        Renderable(
            const std::vector<std::vector<float>>& data,
            const std::vector<uint32_t>& faces,
            const std::vector<int>& bufferIndices,
            const std::shared_ptr<PSO>& pso,
            int verticesCount,
            int facesCount,
            const std::vector<std::shared_ptr<Texture>>& textures
            );

        void render(ICommandEncoder* commandEncoder, const glm::mat4x4& viewProjectionMatrix) const override;


    private:
        std::vector<NS::SharedPtr<MTL::Buffer>> _buffers;
        std::vector<NS::SharedPtr<MTL::Texture>> _textures;
        NS::SharedPtr<MTL::Buffer> _facesBuffer;
        std::vector<int> _bufferIndices;
    };
}

#endif //RENDERABLE_HPP
