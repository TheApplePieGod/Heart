#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Renderer/Buffer.h"
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
        UUID ComputeShaderAsset;
        bool AllowPerformanceQuerying = false;
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

        inline const std::vector<ReflectionDataElement>& GetReflectionData() const { return m_ProgramReflectionData; }

    protected:
        void SortReflectionData();

    protected:
        std::vector<ReflectionDataElement> m_ProgramReflectionData;
    };

    class Texture;
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

    private:
        void ConsolidateReflectionData();
    };

    class ComputePipeline : public Pipeline
    {
    public:
        ComputePipeline(const ComputePipelineCreateInfo& createInfo)
            : m_Info(createInfo)
        {
            HE_ENGINE_ASSERT(createInfo.ComputeShaderAsset != 0, "Must specify a compute shader asset");
            ConsolidateReflectionData();
        }
        virtual ~ComputePipeline() = default;

        virtual void Bind() = 0;

        virtual void BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, u32 elementCount, Buffer* buffer) = 0;
        virtual void BindShaderTextureResource(u32 bindingIndex, Texture* texture) = 0;
        virtual void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex, u32 mipLevel) = 0;
        virtual void FlushBindings() = 0;

        // pipeline must be created with 'AllowPerformanceQuerying' enabled
        double GetPerformanceTimestamp();

        inline u32 GetDispatchCountX() const { return m_DispatchCountX; }
        inline u32 GetDispatchCountY() const { return m_DispatchCountY; }
        inline u32 GetDispatchCountZ() const { return m_DispatchCountZ; }
        inline void SetDispatchCountX(u32 count) { m_DispatchCountX = count; }
        inline void SetDispatchCountY(u32 count) { m_DispatchCountY = count; }
        inline void SetDispatchCountZ(u32 count) { m_DispatchCountZ = count; }
        inline void SetDispatchCount(u32 x, u32 y, u32 z) { m_DispatchCountX = x; m_DispatchCountY = y; m_DispatchCountZ = z; }

    public:
        static Ref<ComputePipeline> Create(const ComputePipelineCreateInfo& createInfo);

    protected:
        ComputePipelineCreateInfo m_Info;
        u32 m_DispatchCountX = 1;
        u32 m_DispatchCountY = 1;
        u32 m_DispatchCountZ = 1;
        double m_PerformanceTimestamp = 0.0;

    private:
        void ConsolidateReflectionData();
    };
}