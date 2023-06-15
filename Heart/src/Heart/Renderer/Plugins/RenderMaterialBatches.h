#pragma once

#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Texture;
    class RenderPass;
    class Framebuffer;
    class ResourceSet;
    class Buffer;
}

namespace Heart::RenderPlugins
{
    struct RenderMaterialBatchesCreateInfo
    {
        HString8 MaterialBatchesPluginName;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        bool CanOutputEntityIds;
    };

    class RenderMaterialBatches : public RenderPlugin
    {
    public:
        RenderMaterialBatches(SceneRenderer2* renderer, HStringView8 name, const RenderMaterialBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline const Flourish::Texture* GetEntityIdsTexture() const { return m_EntityIdsTexture.get(); }
        inline const Flourish::Buffer* GetEntityIdsBuffer() const { return m_EntityIdsBuffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        void Initialize();

    private:
        RenderMaterialBatchesCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_ResourceSet;
        Ref<Flourish::RenderPass> m_RenderPass;
        Ref<Flourish::Framebuffer> m_Framebuffer;
        Ref<Flourish::Texture> m_EntityIdsTexture;
        Ref<Flourish::Texture> m_DefaultEnvironmentMap;
        Ref<Flourish::Buffer> m_EntityIdsBuffer;
    };
}
