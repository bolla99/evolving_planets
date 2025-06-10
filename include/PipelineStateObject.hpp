//
// Created by Giovanni Bollati on 13/03/25.
//

#ifndef PSO_HPP
#define PSO_HPP
#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>
#include <VertexDescriptor.hpp>

class PipelineStateObject {
public:
    PipelineStateObject(
        const std::string& name,
        const std::string& vertexFunctionName,
        const std::string& fragmentFunctionName,
        MTL::Device* device,
        MTL::Library* library,
        MTL::PixelFormat pixelFormat,
        const VertexDescriptor& vertexDescriptor
        );

    VertexDescriptor _vertexDescriptor;

    [[nodiscard]] MTL::RenderPipelineState* getMetalPSO() const { return _metalPSO.get(); }
    [[nodiscard]] MTL::Function* getVertexFunction() const { return _vertexF.get(); }
    [[nodiscard]] MTL::Function* getFragmentFunction() const { return _fragmentF.get(); }

private:
    NS::SharedPtr<MTL::RenderPipelineState> _metalPSO;
    NS::SharedPtr<MTL::Function> _vertexF;
    NS::SharedPtr<MTL::Function> _fragmentF;
};



#endif //PSO_HPP
