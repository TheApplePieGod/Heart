#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec2.hpp"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
    class Texture;
}

namespace Heart::RenderPlugins
{
    struct SVGFCreateInfo
    {
        Ref<Flourish::Texture> InputTexture;
        HString8 FrameDataPluginName;
        HString8 GBufferPluginName;
    };

    class SVGF : public RenderPlugin
    {
    public:
        SVGF(SceneRenderer* renderer, HStringView8 name, const SVGFCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        u32 GetArrayIndex() const;
        u32 GetPrevArrayIndex() const;

        inline const Ref<Flourish::Texture>& GetOutput() const { return m_Output; }
        inline Flourish::Texture* GetColorHistory() const { return m_ColorHistory.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        SVGFCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_TemporalResourceSet;
        Ref<Flourish::ResourceSet> m_ATrousResourceSet;
        Ref<Flourish::ComputePipeline> m_TemporalPipeline;
        Ref<Flourish::ComputePipeline> m_ATrousPipeline;
        Ref<Flourish::Texture> m_Output;
        Ref<Flourish::Texture> m_ColorHistory;
        Ref<Flourish::Texture> m_MomentsHistory;
        Ref<Flourish::Texture> m_TempTexture;

        u32 m_ATrousIterations = 3;
    };
}
