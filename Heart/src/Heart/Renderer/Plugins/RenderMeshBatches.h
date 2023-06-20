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
    struct RenderMeshBatchesCreateInfo
    {
        bool WriteNormals;
        HString8 MeshBatchesPluginName;
        HString8 FrameDataPluginName;
    };

    class RenderMeshBatches : public RenderPlugin
    {
    public:
        RenderMeshBatches(SceneRenderer* renderer, HStringView8 name, const RenderMeshBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline Flourish::Texture* GetNormalsTexture() const { return m_NormalsTexture.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RenderMeshBatchesCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Texture> m_NormalsTexture;
    };
}
