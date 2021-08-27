#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/ShaderInput.h"

namespace Heart
{
    enum class VertexTopology
    {
        TriangleList = 0, PointList = 1, LineList = 2
    };

    enum class CullMode
    {
        None = 0, Backface = 1, Frontface = 2, Both = 3
    };

    struct AttachmentBlendState
    {
        bool BlendEnable;
    };

    struct GraphicsPipelineCreateInfo
    {
        Ref<Shader> VertexShader;
        Ref<Shader> FragmentShader;

        VertexTopology VertexTopology;
        BufferLayout VertexLayout;

        std::vector<AttachmentBlendState> BlendStates; // one state is required per framebuffer attachment
        std::vector<Ref<ShaderInputSet>> CompatibleInputSets;

        bool DepthEnable;
        CullMode CullMode;
    };

    struct ComputePipelineCreateInfo
    {
        
    };

    class Pipeline
    {
    public:
        enum class Type
        {
            None = 0, Graphics = 1, Compute = 2
        };

    public:
        virtual ~Pipeline() = default;
    };

    class GraphicsPipeline : public Pipeline
    {
    public:
        GraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~GraphicsPipeline() = default;

        inline VertexTopology GetVertexTopology() const { return m_Info.VertexTopology; }
        inline CullMode GetCullMode() const { return m_Info.CullMode; }
        inline bool IsDepthEnabled() const { return m_Info.DepthEnable; }
        inline u32 GetVertexLayoutStride() const { return m_Info.VertexLayout.GetStride(); }

    private:
        GraphicsPipelineCreateInfo m_Info;
    };

    class ComputePipeline : public Pipeline
    {
    public:
        ComputePipeline(const ComputePipelineCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~ComputePipeline() = default;

    public:
        static Ref<ComputePipeline> Create(const ComputePipelineCreateInfo& createInfo);

    private:
        ComputePipelineCreateInfo m_Info;
    };
}