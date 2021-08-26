#include "htpch.h"
#include "OpenGLFramebuffer.h"

#include "Heart/Core/Window.h"
#include "imgui/backends/imgui_impl_opengl3.h"

namespace Heart
{
    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
        : Framebuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.Attachments.size() > 0, "Cannot create a framebuffer with zero attachments");

        
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {

    }

    void OpenGLFramebuffer::Bind()
    {
        
    }

    void OpenGLFramebuffer::Submit()
    {

    }

    void OpenGLFramebuffer::BindPipeline(const std::string& name)
    {

    }

    void OpenGLFramebuffer::BindShaderInputSet(const ShaderInputBindPoint& bindPoint, u32 setIndex, const std::vector<u32>& bufferOffsets)
    {

    }

    void* OpenGLFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        return nullptr;
    }

    void* OpenGLFramebuffer::GetDepthAttachmentImGuiHandle()
    {
        return nullptr;
    }

    Ref<GraphicsPipeline> OpenGLFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.Attachments.size(), "Graphics pipeline blend state count must match framebuffer attachment count");

        return nullptr;
    }
}
