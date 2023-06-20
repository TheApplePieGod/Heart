#pragma once

#include "glm/vec3.hpp"
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
        HString8 TransparencyCompositePluginName;
        HString8 SSAOPluginName;
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
        HString8 EntityIdsPluginName; // Optional
    };

    class RenderMaterialBatches : public RenderPlugin
    {
    public:
        struct PBRConfigData
        {
            u32 SSAOEnable = true;
            glm::vec3 Padding;
        };

    public:
        RenderMaterialBatches(SceneRenderer* renderer, HStringView8 name, const RenderMaterialBatchesCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

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
        Ref<Flourish::Buffer> m_DataBuffer;
    };
}
