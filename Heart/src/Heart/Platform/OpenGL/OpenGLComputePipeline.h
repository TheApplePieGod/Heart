#pragma once

#include "Heart/Renderer/Pipeline.h"

namespace Heart
{
    class OpenGLComputePipeline : public ComputePipeline
    {
    public:
        OpenGLComputePipeline(const ComputePipelineCreateInfo& createInfo);
        ~OpenGLComputePipeline() override;

        void Bind() override;

        void BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* buffer)  override;
        void BindShaderTextureResource(u32 bindingIndex, Texture* texture) override;
        void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel) override;
        void FlushBindings() override;

        void Submit();

    private:
        u32 m_ProgramId;

        bool m_FlushedThisFrame = false;
    };
}