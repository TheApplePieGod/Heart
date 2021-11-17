#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Events/WindowEvents.h"
#include "glm/vec4.hpp"

namespace Heart
{
    enum class MsaaSampleCount
    {
        None = 1,
        Two, Four, Eight, Sixteen, Thirtytwo, Sixtyfour,
        Max = Sixtyfour
    };

    struct Subpass
    {
        std::vector<u32> InputAttachmentIndexes;
        std::vector<u32> OutputAttachmentIndexes;
    };

    struct FramebufferAttachment
    {
        glm::vec4 ClearColor;
        ColorFormat Format;
        bool AllowCPURead;
    };

    struct FramebufferCreateInfo
    {
        FramebufferCreateInfo() = default;

        std::vector<FramebufferAttachment> Attachments;
        std::vector<Subpass> Subpasses; // leave empty for no 
        u32 Width, Height = 0; // set to zero to match screen width and height
        MsaaSampleCount SampleCount = MsaaSampleCount::Max; // will be clamped to device max supported sample count
        bool HasDepth = false;
    };

    class Framebuffer : public EventListener
    {
    public:
        Framebuffer(const FramebufferCreateInfo& createInfo);
        virtual ~Framebuffer();

        virtual void Bind() = 0;
        virtual void BindPipeline(const std::string& name) = 0;

        // must be called after BindPipeline()
        virtual void BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, Buffer* buffer) = 0;
        virtual void BindShaderTextureResource(u32 bindingIndex, Texture* texture) = 0;
        
        virtual void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) = 0;
        virtual void* GetDepthAttachmentImGuiHandle() = 0;

        // attachment must be created with 'AllowCPURead' enabled
        virtual void* GetAttachmentPixelData(u32 attachmentIndex) = 0;

        virtual void StartNextSubpass() = 0;

        template<typename T>
        T ReadAttachmentPixel(u32 attachmentIndex, u32 x, u32 y, u32 component)
        {
            T* data = (T*)GetAttachmentPixelData(attachmentIndex);
            u32 index = ColorFormatComponents(m_Info.Attachments[attachmentIndex].Format) * (y * m_ActualWidth + x);
            return data[index + component];
        }

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