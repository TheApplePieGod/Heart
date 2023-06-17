#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class Texture;
    class Buffer;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct SSAOCreateInfo
    {
        HString8 FrameDataPluginName;
        HString8 RenderMeshBatchesPluginName;
    };

    class SSAO : public RenderPlugin
    {
    public:
        struct SSAOData
        {
            glm::vec4 Samples[64];
            u32 KernelSize;
            float Radius;
            float Bias;
            float Padding;
            glm::vec2 RenderSize;
        };

    public:
        SSAO(SceneRenderer* renderer, HStringView8 name, const SSAOCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline const Flourish::Texture* GetOutputTexture() const { return m_OutputTexture.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        SSAOCreateInfo m_Info;

        SSAOData m_Data;
        Ref<Flourish::Buffer> m_DataBuffer;
        Ref<Flourish::Texture> m_NoiseTexture;
        Ref<Flourish::Texture> m_OutputTexture;
        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
    };
}
