#pragma once

#include "glm/vec2.hpp"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Texture;
    class ComputePipeline;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct ColorGradingCreateInfo
    {
        Ref<Flourish::Texture> InputTexture;
        Ref<Flourish::Texture> OutputTexture;
    };

    class ColorGrading : public RenderPlugin
    {
    public:
        ColorGrading(SceneRenderer* renderer, HStringView8 name, const ColorGradingCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct PushConstants
        {
            glm::vec2 SrcResolution;
            glm::vec2 DstResolution;
            u32 TonemapEnable;
        };

    private:
        ColorGradingCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::ComputePipeline> m_Pipeline;
    };
}
