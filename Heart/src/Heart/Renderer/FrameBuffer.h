#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Pipeline.h"
#include "glm/vec4.hpp"

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

    enum class FrameBufferAttachmentType
    {
        None = 0,
        Color, Depth
    };

    struct FrameBufferAttachment
    {
        bool HasDepth = false;
        ColorFormat ColorFormat = ColorFormat::RGBA8;
        glm::vec4 ClearColor; // TODO: resolve clear color?
    };

    struct FrameBufferCreateInfo
    {
        FrameBufferCreateInfo() = default;
		FrameBufferCreateInfo(std::initializer_list<FrameBufferAttachment> attachments)
			: Attachments(attachments) {}

        std::vector<FrameBufferAttachment> Attachments;
        u32 Width, Height = 0; // TODO: set to zero to match screen width and height
        MsaaSampleCount SampleCount = MsaaSampleCount::Max; // will be clamped to device max supported sample count
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
        virtual void BindPipeline(const std::string& name) = 0;
        
        // useful for ImGui
        virtual void* GetRawAttachmentImageHandle(u32 attachmentIndex, FrameBufferAttachmentType type) = 0;

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