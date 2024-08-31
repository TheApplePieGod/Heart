#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Texture;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
}

namespace Heart::RenderPlugins
{
    struct GBufferCreateInfo
    {
        bool KeepHistory;
        u32 MipCount;
        HString8 MeshBatchesPluginName;
        HString8 TextBatchesPluginName;
        HString8 CollectMaterialsPluginName;
        HString8 FrameDataPluginName;
        HString8 EntityIdsPluginName; // Optional
    };

    class GBuffer : public RenderPlugin
    {
    public:
        GBuffer(SceneRenderer* renderer, HStringView8 name, const GBufferCreateInfo& createInfo);

        u32 GetArrayIndex() const;
        u32 GetPrevArrayIndex() const;

        inline Flourish::Texture* GetGBuffer1() const { return m_GBuffer1.get(); }
        inline Flourish::Texture* GetGBuffer2() const { return m_GBuffer2.get(); }
        inline Flourish::Texture* GetGBuffer3() const { return m_GBuffer3.get(); }
        inline Ref<Flourish::Texture>& GetGBufferDepth() { return m_GBufferDepth; }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        GBufferCreateInfo m_Info;

        u32 m_ImageCount = 1;

        Ref<Flourish::ResourceSet> m_StandardResourceSet;
        Ref<Flourish::ResourceSet> m_TextResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        std::array<Ref<Flourish::Framebuffer>, 2> m_Framebuffers;
        Ref<Flourish::Texture> m_GBuffer1;
        Ref<Flourish::Texture> m_GBuffer2;
        Ref<Flourish::Texture> m_GBuffer3;
        Ref<Flourish::Texture> m_GBufferDepth;
    };
}
