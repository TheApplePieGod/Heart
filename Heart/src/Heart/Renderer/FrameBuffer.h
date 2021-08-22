#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/ShaderInput.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Events/WindowEvents.h"
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

    enum class FramebufferAttachmentType
    {
        None = 0,
        Color, Depth
    };

    struct FramebufferAttachment
    {
        bool HasDepth = false;
        ColorFormat ColorFormat = ColorFormat::RGBA8;
        glm::vec4 ClearColor;
    };

    struct FramebufferCreateInfo
    {
        FramebufferCreateInfo() = default;
		FramebufferCreateInfo(std::initializer_list<FramebufferAttachment> attachments)
			: Attachments(attachments) {}

        std::vector<FramebufferAttachment> Attachments;
        u32 Width, Height = 0; // set to zero to match screen width and height
        MsaaSampleCount SampleCount = MsaaSampleCount::Max; // will be clamped to device max supported sample count
    };

    class Framebuffer : public EventListener
    {
    public:
        Framebuffer(const FramebufferCreateInfo& createInfo);
        virtual ~Framebuffer();

        virtual void Bind() = 0;
        virtual void BindPipeline(const std::string& name) = 0;
        virtual void BindShaderInputSet(ShaderInputBindPoint set, u32 setIndex) = 0; // must be called AFTER bind pipeline
        virtual void Submit(GraphicsContext& context) = 0;
        
        // useful for ImGui
        virtual void* GetRawAttachmentImageHandle(u32 attachmentIndex, FramebufferAttachmentType type) = 0;
        void OnEvent(Event& event) override;

        Ref<GraphicsPipeline> RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo);
        Ref<GraphicsPipeline> LoadPipeline(const std::string& name);

        inline u32 GetWidth() const { return m_ActualWidth; }
        inline u32 GetHeight() const { return m_ActualHeight; }

    public:
        static Ref<Framebuffer> Create(const FramebufferCreateInfo& createInfo);

    protected:
        virtual Ref<GraphicsPipeline> InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo) = 0;
        void Invalidate(u32 newWidth, u32 newHeight);

    protected:
        FramebufferCreateInfo m_Info;
        std::unordered_map<std::string, Ref<GraphicsPipeline>> m_GraphicsPipelines;
        bool m_Valid = true;
        u32 m_ActualWidth, m_ActualHeight = 0;

    private:
        bool OnWindowResize(WindowResizeEvent& event);
    };
}