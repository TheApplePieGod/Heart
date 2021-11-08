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
#include "imgui/imgui.h"

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

        // populate cached attachment handles
        m_CachedAttachmentHandles.resize(createInfo.Attachments.size());
        for (size_t i = 0; i < createInfo.Attachments.size(); i++)
            m_CachedAttachmentHandles[i] = GL_COLOR_ATTACHMENT0 + static_cast<int>(i);

        glGenFramebuffers(1, &m_FramebufferId);
        m_ColorAttachmentTextureIds.resize(createInfo.Attachments.size());
        CreateTextures(m_FramebufferId, createInfo.SampleCount, m_ColorAttachmentTextureIds, m_DepthAttachmentTextureId);
        if (createInfo.SampleCount != MsaaSampleCount::None)
        {
            glGenFramebuffers(1, &m_BlitFramebufferId);
            m_BlitColorAttachmentTextureIds.resize(createInfo.Attachments.size());
            CreateTextures(m_BlitFramebufferId, MsaaSampleCount::None, m_BlitColorAttachmentTextureIds, m_BlitDepthAttachmentTextureId);
        }
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        CleanupTextures(m_ColorAttachmentTextureIds, m_DepthAttachmentTextureId);
        if (m_Info.SampleCount != MsaaSampleCount::None)
            CleanupTextures(m_BlitColorAttachmentTextureIds, m_BlitDepthAttachmentTextureId);

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
            int format = OpenGLCommon::ColorFormatToOpenGL(m_Info.Attachments[i].Format);
            glClearTexImage(m_ColorAttachmentTextureIds[i], 0, format, GL_FLOAT, &m_Info.Attachments[i].ClearColor);
            if (m_Info.SampleCount != MsaaSampleCount::None)
                glClearTexImage(m_BlitColorAttachmentTextureIds[i], 0, format, GL_FLOAT, &m_Info.Attachments[i].ClearColor);
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

        // enable draw for all attachments
        glDrawBuffers(static_cast<u32>(m_CachedAttachmentHandles.size()), m_CachedAttachmentHandles.data());
    }

    void OpenGLFramebuffer::Submit()
    {
        // if using a multisampled buffer, perform a framebuffer blit (copy) operation
        if (m_Info.SampleCount != MsaaSampleCount::None)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferId);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BlitFramebufferId);

            // copy all attachments
            for (size_t i = 0; i < m_CachedAttachmentHandles.size(); i++)
            {
                glReadBuffer(m_CachedAttachmentHandles[i]);
                glDrawBuffer(m_CachedAttachmentHandles[i]);
                glBlitFramebuffer(0, 0, m_ActualWidth, m_ActualHeight, 0, 0, m_ActualWidth, m_ActualHeight, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);
            }

            int error = glGetError(); 
            if (error != 0)
            {
                HE_ENGINE_LOG_ERROR("ERROR {0}", error);
                HE_ENGINE_ASSERT(false);
            }   
        }
    }

    void OpenGLFramebuffer::BindPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION();
        
        auto pipeline = static_cast<OpenGLGraphicsPipeline*>(LoadPipeline(name).get());
        glUseProgram(pipeline->GetProgramId());
        glBindVertexArray(pipeline->GetVertexArrayId());
        OpenGLContext::SetBoundGraphicsPipeline(pipeline);
        if (pipeline->GetCullMode() != CullMode::None)
        {
            glEnable(GL_CULL_FACE);
            glCullFace(OpenGLCommon::CullModeToOpenGL(pipeline->GetCullMode()));
        }
        else
            glDisable(GL_CULL_FACE);

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

        if (m_Info.SampleCount == MsaaSampleCount::None)
            return (void*)static_cast<size_t>(m_ColorAttachmentTextureIds[attachmentIndex]);
        else
            return (void*)static_cast<size_t>(m_BlitColorAttachmentTextureIds[attachmentIndex]);
    }

    void* OpenGLFramebuffer::GetDepthAttachmentImGuiHandle()
    {
        HE_ENGINE_ASSERT(m_Info.HasDepth, "Cannot get framebuffer depth attachment handle, HasDepth = false");
        HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot get framebuffer depth attachment handle, SampleCount != None");

        return (void*)static_cast<size_t>(m_DepthAttachmentTextureId);
    }

    Ref<GraphicsPipeline> OpenGLFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == m_Info.Attachments.size(), "Graphics pipeline blend state count must match framebuffer attachment count");

        return CreateRef<OpenGLGraphicsPipeline>(createInfo);
    }

    void OpenGLFramebuffer::CreateTextures(int framebufferId, MsaaSampleCount sampleCount, std::vector<u32>& attachmentArray, u32& depthAttachment)
    {
        int textureTarget = sampleCount != MsaaSampleCount::None ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

        glCreateTextures(textureTarget, static_cast<int>(attachmentArray.size()), attachmentArray.data());
        
        if (m_Info.HasDepth)
            glGenTextures(1, &depthAttachment);

        glBindFramebuffer(GL_FRAMEBUFFER, framebufferId);

        int actualSampleCount = OpenGLCommon::MsaaSampleCountToOpenGL(sampleCount);
        if (OpenGLContext::MaxMsaaSamples() < actualSampleCount)
            actualSampleCount = OpenGLContext::MaxMsaaSamples();

        for (size_t i = 0; i < m_Info.Attachments.size(); i++)
        {
            int actualFormat = OpenGLCommon::ColorFormatToOpenGL(m_Info.Attachments[i].Format);
            int actualFormatInternal = OpenGLCommon::ColorFormatToInternalOpenGL(m_Info.Attachments[i].Format);

            glBindTexture(textureTarget, attachmentArray[i]);

            if (sampleCount != MsaaSampleCount::None)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, actualSampleCount, actualFormatInternal, m_ActualWidth, m_ActualHeight, GL_FALSE);
            else
            {
                glTexImage2D(GL_TEXTURE_2D, 0, actualFormatInternal, m_ActualWidth, m_ActualHeight, 0, actualFormat, GL_UNSIGNED_BYTE, NULL);

                // TODO: dynamic filtering
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + static_cast<int>(i), textureTarget, attachmentArray[i], 0);
        }

        if (m_Info.HasDepth)
        {
            glBindTexture(textureTarget, depthAttachment);

            if (sampleCount != MsaaSampleCount::None)
                glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, actualSampleCount, m_DepthFormatInternal, m_ActualWidth, m_ActualHeight, GL_FALSE);
            else
            {
                glTexStorage2D(GL_TEXTURE_2D, 1, m_DepthFormatInternal, m_ActualWidth, m_ActualHeight);

                // TODO: dynamic filtering
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            }

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, textureTarget, depthAttachment, 0);  
        }

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
        {
            HE_ENGINE_LOG_CRITICAL("Failed to create OpenGL framebuffer: {0}", glCheckFramebufferStatus(GL_FRAMEBUFFER));
            HE_ENGINE_ASSERT(false);
        }
    }

    void OpenGLFramebuffer::CleanupTextures(std::vector<u32>& attachmentArray, u32& depthAttachment)
    {
        glDeleteTextures(static_cast<int>(attachmentArray.size()), attachmentArray.data());

        if (m_Info.HasDepth)
            glDeleteTextures(1, &depthAttachment);
    }

    void OpenGLFramebuffer::Recreate()
    {
        glDeleteFramebuffers(1, &m_FramebufferId);
        glGenFramebuffers(1, &m_FramebufferId);

        CleanupTextures(m_ColorAttachmentTextureIds, m_DepthAttachmentTextureId);
        CreateTextures(m_FramebufferId, m_Info.SampleCount, m_ColorAttachmentTextureIds, m_DepthAttachmentTextureId);

        if (m_Info.SampleCount != MsaaSampleCount::None)
        {
            glDeleteFramebuffers(1, &m_BlitFramebufferId);
            glGenFramebuffers(1, &m_BlitFramebufferId);

            CleanupTextures(m_BlitColorAttachmentTextureIds, m_BlitDepthAttachmentTextureId);
            CreateTextures(m_BlitFramebufferId, MsaaSampleCount::None, m_BlitColorAttachmentTextureIds, m_BlitDepthAttachmentTextureId);
        }    

        m_Valid = true;
    }
}
