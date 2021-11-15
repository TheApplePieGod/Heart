#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"

namespace Heart
{
    enum class VertexTopology
    {
        None = 0,
        TriangleList, TriangleStrip, TriangleFan, PointList, LineList, LineStrip
    };

    enum class CullMode
    {
        None = 0,
        Backface, Frontface, BackAndFront
    };

    enum class WindingOrder
    {
        None = 0,
        Clockwise, CounterClockwise
    };

    struct AttachmentBlendState
    {
        bool BlendEnable;
    };

    struct GraphicsPipelineCreateInfo
    {
        std::string VertexShaderPath;
        std::string FragmentShaderPath;

        VertexTopology VertexTopology;
        BufferLayout VertexLayout;

        std::vector<AttachmentBlendState> BlendStates; // one state is required per framebuffer attachment

        bool DepthEnable;
        CullMode CullMode;
        WindingOrder WindingOrder;
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
        {
            HE_ENGINE_ASSERT(createInfo.VertexShaderPath != "" && createInfo.FragmentShaderPath != "", "Must specify both vertex and fragment shaders");
            ConsolidateReflectionData();
        }
        virtual ~GraphicsPipeline() = default;

        inline VertexTopology GetVertexTopology() const { return m_Info.VertexTopology; }
        inline CullMode GetCullMode() const { return m_Info.CullMode; }
        inline WindingOrder GetWindingOrder() const { return m_Info.WindingOrder; }
        inline bool IsDepthEnabled() const { return m_Info.DepthEnable; }
        inline u32 GetVertexLayoutStride() const { return m_Info.VertexLayout.GetStride(); }
        inline const std::vector<AttachmentBlendState>& GetBlendStates() const { return m_Info.BlendStates; }

    protected:
        GraphicsPipelineCreateInfo m_Info;
        std::vector<ReflectionDataElement> m_ProgramReflectionData; // program = vertex+fragment shaders

    private:
        void ConsolidateReflectionData();
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

    protected:
        ComputePipelineCreateInfo m_Info;
    };
}