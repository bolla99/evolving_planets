//
// Created by Giovanni Bollati on 13/03/25.
//

#ifndef RENDERABLE_HPP
#define RENDERABLE_HPP
#include "Metal/MTLRenderCommandEncoder.hpp"
#include <simd/simd.h>
#include "PSO.hpp"
#include "MeshPSO.hpp"
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
            int verticesCount,
            int facesCount,
            const std::vector<std::shared_ptr<Texture>>& textures,
            const std::vector<std::vector<std::pair<Core::VertexAttributeName, Core::VertexAttributeType>>>& vertexAttributes,
            const std::vector<Core::TextureType>& texturesTypes,
            MTL::Device* device
            );

        // override render function
        void render(
            ICommandEncoder* commandEncoder,
            IPSO* pso,
            uint instanceCount,
            DispatchSize gridSize = {0, 0, 0},
            DispatchSize threadgroupSize = {0, 0, 0}
        ) const override;

        void setMetalTexture(const std::vector<NS::SharedPtr<MTL::Texture>>& texture, const std::vector<TextureType>& textureTypes)
        {
            _textures = texture;
            _texturesTypes = textureTypes;
        }

    private:
        // metal data
        std::vector<NS::SharedPtr<MTL::Buffer>> _buffers;
        std::vector<NS::SharedPtr<MTL::Texture>> _textures;
        NS::SharedPtr<MTL::Buffer> _facesBuffer;

        // Helper methods for rendering
        void bindSamplers(MTL::RenderCommandEncoder* encoder, PSO* pso, const std::vector<Core::SamplerBinding>& samplers) const;
        void bindSamplers(MTL::RenderCommandEncoder* encoder, MeshPSO* pso, const std::vector<Core::SamplerBinding>& samplers) const;
        void bindTextures(MTL::RenderCommandEncoder* encoder, const PSOConfig& config) const;
    };
}

#endif //RENDERABLE_HPP
