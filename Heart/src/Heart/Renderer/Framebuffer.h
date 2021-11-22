#pragma once

#include "Heart/Renderer/GraphicsContext.h"
#include "Heart/Renderer/Pipeline.h"
#include "Heart/Renderer/Texture.h"
#include "Heart/Events/EventEmitter.h"
#include "Heart/Events/WindowEvents.h"
#include "glm/vec4.hpp"
#include "glm/vec2.hpp"

namespace Heart
{
    enum class MsaaSampleCount
    {
        None = 1,
        Two, Four, Eight, Sixteen, Thirtytwo, Sixtyfour,
        Max = Sixtyfour
    };

    enum class SubpassAttachmentType
    {
        None = 0,
        Color, Depth
    };

    struct SubpassAttachment
    {
        SubpassAttachmentType Type;
        u32 AttachmentIndex;
    };

    struct Subpass
    {
        std::vector<SubpassAttachment> InputAttachments;
        std::vector<SubpassAttachment> OutputAttachments;
    };

    struct FramebufferColorAttachment
    {
        // mandatory
        bool AllowCPURead;
        glm::vec4 ClearColor;

        // required if not providing a texture
        ColorFormat Format;

        // required if a texture should be used as a render target
        Ref<Texture> Texture;
        u32 LayerIndex;
        u32 MipLevel;
    };

    struct FramebufferDepthAttachment
    {};

    struct FramebufferCreateInfo
    {
        FramebufferCreateInfo() = default;

        std::vector<FramebufferColorAttachment> ColorAttachments;
        std::vector<FramebufferDepthAttachment> DepthAttachments;
        std::vector<Subpass> Subpasses; // leave empty for no 
        u32 Width, Height = 0; // set to zero to match screen width and height
        MsaaSampleCount SampleCount = MsaaSampleCount::None; // will be clamped to device max supported sample count
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
        virtual void BindShaderTextureLayerResource(u32 bindingIndex, Texture* texture, u32 layerIndex) = 0;
        virtual void BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment) = 0;

        virtual void* GetColorAttachmentImGuiHandle(u32 attachmentIndex) = 0;

        // attachment must be created with 'AllowCPURead' enabled
        virtual void* GetColorAttachmentPixelData(u32 attachmentIndex) = 0;

        virtual void ClearOutputAttachment(u32 outputAttachmentIndex, bool clearDepth) = 0;
        virtual void StartNextSubpass() = 0;

        template<typename T>
        T ReadColorAttachmentPixel(u32 attachmentIndex, u32 x, u32 y, u32 component)
        {
            T* data = (T*)GetColorAttachmentPixelData(attachmentIndex);
            u32 index = ColorFormatComponents(m_Info.ColorAttachments[attachmentIndex].Format) * (y * m_ActualWidth + x);
            return data[index + component];
        }

        void OnEvent(Event& event) override;

        Ref<GraphicsPipeline> RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo);
        Ref<GraphicsPipeline> LoadPipeline(const std::string& name);

        inline u32 GetWidth() const { return m_ActualWidth; }
        inline u32 GetHeight() const { return m_ActualHeight; }
        inline glm::vec2 GetSize() const { return { m_ActualWidth, m_ActualHeight }; }

        u32 GetSubpassOutputColorAttachmentCount(u32 subpassIndex) const
        { 
            u32 count = 0;
            for (auto& attachment : m_Info.Subpasses[subpassIndex].OutputAttachments)
                if (attachment.Type == SubpassAttachmentType::Color)
                    count++;
            return count;
        }
        bool HasOutputDepthAttachment(u32 subpassIndex) const
        { 
            u32 count = 0;
            for (auto& attachment : m_Info.Subpasses[subpassIndex].OutputAttachments)
                if (attachment.Type == SubpassAttachmentType::Depth)
                    return true;
            return false;
        }

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