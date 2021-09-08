#include "htpch.h"
#include "OpenGLFramebuffer.h"

#include "glad/glad.h"
#include "Heart/Core/Window.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/OpenGL/OpenGLGraphicsPipeline.h"
#include "Heart/Platform/OpenGL/OpenGLContext.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "imgui/backends/imgui_impl_opengl3.h"

namespace Heart
{
    const int m_DepthFormat = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    const int m_DepthFormatInternal = GL_DEPTH32F_STENCIL8;

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
        : Framebuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.Attachments.size() > 0, "Cannot create a framebuffer with zero attachments");
        HE_ENGINE_ASSERT(createInfo.Attachments.size() <= 30, "Cannot create a framebuffer with more than 30 attachments");
        Window& mainWindow = Window::GetMainWindow();

        m_ActualWidth = createInfo.Width == 0 ? mainWindow.GetWidth() : createInfo.Width;
        m_ActualHeight = createInfo.Height == 0 ? mainWindow.GetHeight() : createInfo.Height;

        glGenFramebuffers(1, &m_FramebufferId);

        m_ColorAttachmentTextureIds.resize(createInfo.Attachments.size());

        CreateTextures();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        CleanupTextures();

        glDeleteFramebuffers(1, &m_FramebufferId);
    }

    void OpenGLFramebuffer::Bind()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Valid)
            Recreate();

        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);
        glViewport(0, 0, m_ActualWidth, m_ActualHeight);
        
        // clear each attachment with the provided color
        for (size_t i = 0; i < m_Info.Attachments.size(); i++)
        {
            glClearTexImage(m_ColorAttachmentTextureIds[i], 0, GL_RGBA, GL_FLOAT, &m_Info.Attachments[i].ClearColor);
        }

        if (m_Info.HasDepth)
        {
            if (Renderer::IsUsingReverseDepth())
                glClearDepth(0.0f);
            else
                glClearDepth(1.0f);
            glClear(GL_STENCIL_BUFFER_BIT);
            glClear(GL_DEPTH_BUFFER_BIT);
        }
    }

    void OpenGLFramebuffer::Submit()
    {
        
    }

    void OpenGLFramebuffer::BindPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION();
        
        auto pipeline = static_cast<OpenGLGraphicsPipeline*>(LoadPipeline(name).get());
        glUseProgram(pipeline->GetProgramId());
        glBindVertexArray(pipeline->GetVertexArrayId());
        OpenGLContext::SetBoundGraphicsPipeline(pipeline);
        glCullFace(OpenGLCommon::CullModeToOpenGL(pipeline->GetCullMode()));
        if (pipeline->IsDepthEnabled())
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
        if (Renderer::IsUsingReverseDepth())
            glDepthFunc(GL_GEQUAL);
        else
            glDepthFunc(GL_LESS);
    }

    void OpenGLFramebuffer::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, Buffer* _buffer)
    {
        HE_PROFILE_FUNCTION();

        OpenGLBuffer& buffer = static_cast<OpenGLBuffer&>(*_buffer);

        glBindBufferBase(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId());
        glBindBufferRange(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId(), elementOffset * buffer.GetLayout().GetStride(), buffer.GetAllocatedSize());
    }

    void OpenGLFramebuffer::BindShaderTextureResource(u32 bindingIndex, Texture* _texture)
    {
        HE_PROFILE_FUNCTION();

        OpenGLTexture& texture = static_cast<OpenGLTexture&>(*_texture);

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        glBindTexture(GL_TEXTURE_2D, texture.GetTextureId());
    }

    void* OpenGLFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_ColorAttachmentTextureIds.size(), "Attachment access on framebuffer out of range");

        return (void*)static_cast<size_t>(m_ColorAttachmentTextureIds[attachmentIndex]);
    }

    void* OpenGLFramebuffer::GetDepthAttachmentImGuiHandle()
    {
        HE_ENGINE_ASSERT(m_Info.HasDepth, "Cannot get framebuffer depth attachment handle, HasDepth = false");

        return (void*)static_cast<size_t>(m_DepthAttachmentTextureId);
    }

    Ref<GraphicsPipeline> OpenGLFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.Attachments.size(), "Graphics pipeline blend state count must match framebuffer attachment count");

        return CreateRef<OpenGLGraphicsPipeline>(createInfo);
    }

    void OpenGLFramebuffer::CreateTextures()
    {
        glGenTextures(static_cast<int>(m_ColorAttachmentTextureIds.size()), m_ColorAttachmentTextureIds.data());

        if (m_Info.HasDepth)
            glGenTextures(1, &m_DepthAttachmentTextureId);

        glBindFramebuffer(GL_FRAMEBUFFER, m_FramebufferId);

        for (size_t i = 0; i < m_Info.Attachments.size(); i++)
        {
            glBindTexture(GL_TEXTURE_2D, m_ColorAttachmentTextureIds[i]);

            if (m_Info.SampleCount != MsaaSampleCount::None)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, OpenGLCommon::MsaaSampleCountToOpenGL(m_Info.SampleCount), GL_RGBA8, m_ActualWidth, m_ActualHeight, GL_FALSE);

            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_ActualWidth, m_ActualHeight, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);

            // TODO: dynamic filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<int>(i), GL_TEXTURE_2D, m_ColorAttachmentTextureIds[i], 0);  
        }

        if (m_Info.HasDepth)
        {
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentTextureId);

            if (m_Info.SampleCount != MsaaSampleCount::None)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, OpenGLCommon::MsaaSampleCountToOpenGL(m_Info.SampleCount), m_DepthFormatInternal, m_ActualWidth, m_ActualHeight, GL_FALSE);

            //glTexImage2D(GL_TEXTURE_2D, 0, m_DepthFormatInternal, m_ActualWidth, m_ActualHeight, 0, GL_DEPTH_STENCIL, m_DepthFormat , NULL);
            glTexStorage2D(GL_TEXTURE_2D, 1, m_DepthFormatInternal, m_ActualWidth, m_ActualHeight);

            // TODO: dynamic filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachmentTextureId, 0);  
        }
    }

    void OpenGLFramebuffer::CleanupTextures()
    {
        glDeleteTextures(static_cast<int>(m_ColorAttachmentTextureIds.size()), m_ColorAttachmentTextureIds.data());

        if (m_Info.HasDepth)
            glDeleteTextures(1, &m_DepthAttachmentTextureId);
    }

    void OpenGLFramebuffer::Recreate()
    {
        glDeleteFramebuffers(1, &m_FramebufferId);
        glGenFramebuffers(1, &m_FramebufferId);

        CleanupTextures();
        CreateTextures();

        m_Valid = true;
    }
}
