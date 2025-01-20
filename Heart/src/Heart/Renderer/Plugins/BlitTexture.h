#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Heart::RenderPlugins
{
    struct BlitTextureCreateInfo
    {
        Ref<Flourish::Texture> SrcTexture;
        bool SrcDynamicLayerIndex = false;
        u32 SrcLayerIndex = 0;
        u32 SrcMipLevel = 0;

        Ref<Flourish::Texture> DstTexture;
        bool DstDynamicLayerIndex = false;
        u32 DstLayerIndex = 0;
        u32 DstMipLevel = 0;
    };

    class BlitTexture : public RenderPlugin
    {
    public:
        BlitTexture(SceneRenderer* renderer, HStringView8 name, const BlitTextureCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        BlitTextureCreateInfo m_Info;
    };
}
