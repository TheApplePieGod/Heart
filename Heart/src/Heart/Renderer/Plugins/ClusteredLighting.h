#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "glm/vec4.hpp"

namespace Flourish
{
    class ResourceSet;
    class ComputePipeline;
    class Buffer;
}

namespace Heart::RenderPlugins
{
    struct ClusteredLightingCreateInfo
    {
        HString8 FrameDataPluginName;
        HString8 LightingDataPluginName;
    };

    class ClusteredLighting : public RenderPlugin
    {
    public:
        struct ClusterData
        {
            glm::vec4 ClusterDims;
            float ClusterScale;
            float ClusterBias;
        };

    public:
        ClusteredLighting(SceneRenderer* renderer, HStringView8 name, const ClusteredLightingCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        {}

        inline Flourish::Buffer* GetClusterBuffer() const { return m_ClusterBuffer.get(); }
        inline Flourish::Buffer* GetLightIndicesBuffer() const { return m_LightIndicesBuffer.get(); }
        inline Flourish::Buffer* GetLightGridBuffer() const { return m_LightGridBuffer.get(); }
        inline Flourish::Buffer* GetClusterDataBuffer() const { return m_ClusterData.get(); }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        struct PushData
        {
            glm::vec4 ClusterDims;
        };

        struct Cluster
        {
            glm::vec4 MinBounds;
            glm::vec4 MaxBounds;
        };

    private:
        ClusteredLightingCreateInfo m_Info;

        Ref<Flourish::ResourceSet> m_BuildResourceSet;
        Ref<Flourish::ResourceSet> m_CullResourceSet;
        Ref<Flourish::ComputePipeline> m_BuildPipeline;
        Ref<Flourish::ComputePipeline> m_CullPipeline;
        Ref<Flourish::Buffer> m_ClusterBuffer;
        Ref<Flourish::Buffer> m_LightIndicesBuffer;
        Ref<Flourish::Buffer> m_LightGridBuffer;
        Ref<Flourish::Buffer> m_BuildData;
        Ref<Flourish::Buffer> m_ClusterData;

        PushData m_PushData;
    };
}
