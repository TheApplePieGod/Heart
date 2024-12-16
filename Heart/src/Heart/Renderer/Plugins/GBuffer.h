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
        bool KeepHistory; // Save normals + depth for 1 extra frame
        u32 MipCount;
        bool StoreMotionVectors;
        bool StoreColorAndEmissiveData;
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

        inline Ref<Flourish::Texture>& GetNormalData() { return m_NormalData; }
        inline Ref<Flourish::Texture>& GetColorData() { return m_ColorData; }
        inline Ref<Flourish::Texture>& GetEmissiveData() { return m_EmissiveData; }
        inline Ref<Flourish::Texture>& GetDepth() { return m_Depth; }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct PushData
        {
            u32 StoreMotionVectors;
            u32 StoreColorAndEmissive;
            u32 StoreEntityIds;
        };

    private:
        GBufferCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_StandardResourceSet;
        Ref<Flourish::ResourceSet> m_TextResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        std::array<Ref<Flourish::Framebuffer>, 2> m_Framebuffers;
        Ref<Flourish::Texture> m_NormalData; // RG: octahedron WS normal vector, BA: motion vector 
        Ref<Flourish::Texture> m_ColorData; // RGB: Albedo, A: Packed metalness/roughness
        Ref<Flourish::Texture> m_EmissiveData; // RGB: Emissive, A: Occlusion
        Ref<Flourish::Texture> m_Depth;

        u32 m_ImageCount = 1;
        PushData m_PushData;
    };
}
