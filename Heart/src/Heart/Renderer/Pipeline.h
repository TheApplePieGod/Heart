#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Core/UUID.h"

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

    enum class BlendFactor
    {
        Zero = 0,
        One,
        SrcColor, OneMinusSrcColor, DstColor, OneMinusDstColor,
        SrcAlpha, OneMinusSrcAlpha, DstAlpha, OneMinusDstAlpha
    };

    enum class BlendOperation
    {
        Add = 0,
        Subtract, ReverseSubtract, Min, Max
    };

    struct AttachmentBlendState
    {
        bool BlendEnable;
        BlendFactor SrcColorBlendFactor = BlendFactor::SrcAlpha;
        BlendFactor DstColorBlendFactor = BlendFactor::OneMinusSrcAlpha;
        BlendFactor SrcAlphaBlendFactor = BlendFactor::One;
        BlendFactor DstAlphaBlendFactor = BlendFactor::Zero;
        BlendOperation ColorBlendOperation = BlendOperation::Add;
        BlendOperation AlphaBlendOperation = BlendOperation::Add;
    };

    struct GraphicsPipelineCreateInfo
    {
        UUID VertexShaderAsset;
        UUID FragmentShaderAsset;

        bool VertexInput;
        VertexTopology VertexTopology;
        BufferLayout VertexLayout;

        std::vector<AttachmentBlendState> BlendStates; // one state is required per framebuffer attachment

        bool DepthTest;
        bool DepthWrite;
        CullMode CullMode;
        WindingOrder WindingOrder;
        u32 SubpassIndex;
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
            HE_ENGINE_ASSERT(createInfo.VertexShaderAsset != 0 && createInfo.FragmentShaderAsset != 0, "Must specify both vertex and fragment shaders");
            ConsolidateReflectionData();
        }
        virtual ~GraphicsPipeline() = default;

        inline VertexTopology GetVertexTopology() const { return m_Info.VertexTopology; }
        inline CullMode GetCullMode() const { return m_Info.CullMode; }
        inline WindingOrder GetWindingOrder() const { return m_Info.WindingOrder; }
        inline bool IsDepthTestEnabled() const { return m_Info.DepthTest; }
        inline bool IsDepthWriteEnabled() const { return m_Info.DepthWrite; }
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