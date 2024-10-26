#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class Texture;
    class Buffer;
    class ComputePipeline;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct SSAOCreateInfo
    {
        HString8 FrameDataPluginName;

        Ref<Flourish::Texture> InputDepthTexture;
        Ref<Flourish::Texture> InputNormalsTexture;
    };

    class SSAO : public RenderPlugin
    {
    public:
        SSAO(SceneRenderer* renderer, HStringView8 name, const SSAOCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

        inline const Flourish::Texture* GetOutputTexture() const { return m_OutputTexture.get(); }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct PushConstants
        {
            glm::vec4 Samples[64];
            u32 KernelSize;
            float Radius;
            float Bias;
            float Padding;
            glm::vec2 RenderSize;
        };

    private:
        SSAOCreateInfo m_Info;

        PushConstants m_PushConstants;
        Ref<Flourish::Texture> m_NoiseTexture;
        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ComputePipeline> m_Pipeline;
    };
}
