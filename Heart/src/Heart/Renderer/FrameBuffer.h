#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Pipeline.h"

namespace Heart
{
    // normal maps should be loaded with the UNORM specifier
    enum class ColorFormat
    {
        R8 = 0,
        RG8, RGB8, RGBA8, RGBA32
    };

    enum class MsaaSampleCount
    {
        None = 1,
        Two, Four, Eight, Sixteen, Thirtytwo, Sixtyfour,
        Max = Sixtyfour
    };

    struct FrameBufferAttachment
    {
        bool HasDepth = false;
        bool ForceResolve = false; // the attachment will automatically have a resolve if SampleCount is > none   
        ColorFormat ColorFormat = ColorFormat::RGBA8;
        u32 ResolveWidth, ResolveHeight = 0; // keep these at zero to match Width and Height
    };

    struct FrameBufferCreateInfo
    {
        FrameBufferCreateInfo() = default;
		FrameBufferCreateInfo(std::initializer_list<FrameBufferAttachment> attachments)
			: Attachments(attachments) {}

        std::vector<FrameBufferAttachment> Attachments;
        u32 Width, Height = 0; // TODO: set to zero to match screen width and height
        MsaaSampleCount SampleCount = MsaaSampleCount::Max;
    };

    class FrameBuffer
    {
    public:
        FrameBuffer(const FrameBufferCreateInfo& createInfo)
            : m_Info(createInfo)
        {}
        virtual ~FrameBuffer() = default;

        virtual void Bind() = 0;
        virtual void Submit(GraphicsContext& context) = 0;

        Ref<GraphicsPipeline> RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo);
        Ref<GraphicsPipeline> LoadPipeline(const std::string& name);

    public:
        static Ref<FrameBuffer> Create(const FrameBufferCreateInfo& createInfo);

    protected:
        virtual Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) = 0;

    protected:
        FrameBufferCreateInfo m_Info;
        std::unordered_map<std::string, Ref<GraphicsPipeline>> m_GraphicsPipelines;
    };
}