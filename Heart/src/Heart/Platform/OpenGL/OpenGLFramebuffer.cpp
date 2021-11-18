#include "htpch.h"
#include "OpenGLFramebuffer.h"

#include "glad/glad.h"
#include "Heart/Core/App.h"
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
    const ColorFormat m_GeneralDepthFormat = ColorFormat::R32F;
    const int m_DepthFormat = GL_FLOAT_32_UNSIGNED_INT_24_8_REV;
    const int m_DepthFormatInternal = GL_DEPTH32F_STENCIL8;

    OpenGLFramebuffer::OpenGLFramebuffer(const FramebufferCreateInfo& createInfo)
        : Framebuffer(createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.ColorAttachments.size() > 0, "Cannot create a framebuffer with zero attachments");
        Window& mainWindow = Window::GetMainWindow();

        m_ActualWidth = createInfo.Width == 0 ? mainWindow.GetWidth() : createInfo.Width;
        m_ActualHeight = createInfo.Height == 0 ? mainWindow.GetHeight() : createInfo.Height;

        // populate cached attachment handles
        m_CachedAttachmentHandles.resize(createInfo.ColorAttachments.size());
        for (size_t i = 0; i < createInfo.ColorAttachments.size(); i++)
            m_CachedAttachmentHandles[i] = GL_COLOR_ATTACHMENT0 + static_cast<int>(i);

        m_PixelBufferObjects.resize(createInfo.ColorAttachments.size());
        m_PixelBufferMappings.resize(createInfo.ColorAttachments.size());
        
        m_ImageSamples = OpenGLCommon::MsaaSampleCountToOpenGL(createInfo.SampleCount);
        if (m_ImageSamples > OpenGLContext::MaxMsaaSamples())
            m_ImageSamples = OpenGLContext::MaxMsaaSamples();

        for (auto& attachment : m_Info.DepthAttachments)
        {
            OpenGLFramebufferAttachment attachmentData;
            attachmentData.GeneralColorFormat = m_GeneralDepthFormat;
            attachmentData.ColorFormat = m_DepthFormat;
            attachmentData.ColorFormatInternal = m_DepthFormatInternal;
            attachmentData.HasResolve = m_ImageSamples > 1; // createInfo.SampleCount != MsaaSampleCount::None;
            attachmentData.CPUVisible = attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = true;

            CreateAttachmentTextures(attachmentData);

            m_DepthAttachmentData.emplace_back(attachmentData);
        }

        for (auto& attachment : m_Info.ColorAttachments)
        {
            OpenGLFramebufferAttachment attachmentData;
            attachmentData.GeneralColorFormat = attachment.Format;
            attachmentData.ColorFormat = OpenGLCommon::ColorFormatToOpenGL(attachment.Format);
            attachmentData.ColorFormatInternal = OpenGLCommon::ColorFormatToInternalOpenGL(attachment.Format);
            attachmentData.HasResolve = m_ImageSamples > 1;
            attachmentData.CPUVisible = attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = false;

            CreateAttachmentTextures(attachmentData);

            m_AttachmentData.emplace_back(attachmentData);
        }

        m_Framebuffers.resize(createInfo.Subpasses.size());
        if (m_ImageSamples > 1)
            m_BlitFramebuffers.resize(createInfo.Subpasses.size());

        CreateFramebuffers();

        CreatePixelBuffers();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        CleanupFramebuffers();

        for (auto& attachmentData : m_AttachmentData)
            CleanupAttachmentTextures(attachmentData);
        for (auto& attachmentData : m_DepthAttachmentData)
            CleanupAttachmentTextures(attachmentData);

        CleanupPixelBuffers();
    }

    void OpenGLFramebuffer::Bind()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Valid)
            Recreate();

        if (m_CurrentSubpass == -1)
        {
            StartNextSubpass();
        
            // clear each attachment with the provided color
            for (size_t i = 0; i < m_AttachmentData.size(); i++)
            {
                glClearTexImage(m_AttachmentData[i].Image, 0, m_AttachmentData[i].ColorFormat, GL_FLOAT, &m_Info.ColorAttachments[i].ClearColor);
                //if (m_ImageSamples > 1)
                //    glClearTexImage(m_AttachmentData[i].BlitImage, 0, m_AttachmentData[i].ColorFormat, GL_FLOAT, &m_Info.ColorAttachments[i].ClearColor);
            }

            // kindof a hack
            // in order to clear all the depth buffers we will bind every framebuffer and clear them through there
            float clearColor = Renderer::IsUsingReverseDepth() ? 0.f : 1.f;
            glDepthMask(GL_TRUE);
            if (Renderer::IsUsingReverseDepth())
                glClearDepth(0.f);
            else
                glClearDepth(1.f);
            for (size_t i = 0; i < m_Framebuffers.size(); i++)
            {
                glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
                glClear(GL_DEPTH_BUFFER_BIT);
                glClear(GL_STENCIL_BUFFER_BIT);
            }
        }
    }

    void OpenGLFramebuffer::Submit()
    {
        BlitFramebuffers(m_CurrentSubpass);

        for (size_t i = 0; i < m_Info.ColorAttachments.size(); i++)
        {
            // unmap any buffers that were mapped this frame
            if (m_Info.ColorAttachments[i].AllowCPURead && m_PixelBufferMappings[i] != nullptr)
            {
                glUnmapNamedBuffer(m_PixelBufferObjects[i][(App::Get().GetFrameCount() + 1) % 2]->GetBufferId());
                m_PixelBufferMappings[i] = nullptr;
            }

            // start the async read for cpu visible attachments
            // if (m_Info.ColorAttachments[i].AllowCPURead)
            // {
            //     if (m_Info.SampleCount != MsaaSampleCount::None)
            //         glBindFramebuffer(GL_READ_FRAMEBUFFER, m_BlitFramebufferId);
            //     else
            //         glBindFramebuffer(GL_READ_FRAMEBUFFER, m_FramebufferId);
            //     glReadBuffer(m_CachedAttachmentHandles[i]);
            //     glBindBuffer(GL_PIXEL_PACK_BUFFER, m_PixelBufferObjects[i][App::Get().GetFrameCount() % 2]->GetBufferId());
            //     glReadPixels(0, 0, m_ActualWidth, m_ActualHeight, OpenGLCommon::ColorFormatToOpenGL(m_Info.ColorAttachments[i].Format), OpenGLCommon::ColorFormatToOpenGLDataType(m_Info.ColorAttachments[i].Format), nullptr);
            // }

            glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
        }

        // int error = glGetError(); 
        // if (error != 0)
        // {
        //     HE_ENGINE_LOG_ERROR("ERROR {0}", error);
        //     HE_ENGINE_ASSERT(false);
        // }

        m_CurrentSubpass = -1;
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

        glFrontFace(OpenGLCommon::WindingOrderToOpenGL(pipeline->GetWindingOrder()));

        if (pipeline->IsDepthTestEnabled())
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);

        if (pipeline->IsDepthWriteEnabled())
            glDepthMask(GL_TRUE);
        else
            glDepthMask(GL_FALSE);
            
        if (Renderer::IsUsingReverseDepth())
            glDepthFunc(GL_GEQUAL);
        else
            glDepthFunc(GL_LESS);

        for (size_t i = 0; i < GetSubpassOutputColorAttachmentCount(m_CurrentSubpass); i++)
        {
            int buffer = static_cast<int>(i);
            auto& blendState = pipeline->GetBlendStates()[i];
            if (blendState.BlendEnable)
            {
                glEnablei(GL_BLEND, buffer);
                glBlendEquationSeparatei(
                    buffer,
                    OpenGLCommon::BlendOperationToOpenGL(blendState.ColorBlendOperation),
                    OpenGLCommon::BlendOperationToOpenGL(blendState.AlphaBlendOperation)
                );
                glBlendFuncSeparatei(
                    buffer,
                    OpenGLCommon::BlendFactorToOpenGL(blendState.SrcColorBlendFactor),
                    OpenGLCommon::BlendFactorToOpenGL(blendState.DstColorBlendFactor),
                    OpenGLCommon::BlendFactorToOpenGL(blendState.SrcAlphaBlendFactor),
                    OpenGLCommon::BlendFactorToOpenGL(blendState.DstAlphaBlendFactor)
                );
            }
            else
                glDisablei(GL_BLEND, buffer);
        }
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

    void OpenGLFramebuffer::BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment)
    {
        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        if (attachment.Type == SubpassAttachmentType::Depth)
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentData[attachment.AttachmentIndex].HasResolve ? m_DepthAttachmentData[attachment.AttachmentIndex].BlitImage : m_DepthAttachmentData[attachment.AttachmentIndex].Image);
        else
            glBindTexture(GL_TEXTURE_2D, m_AttachmentData[attachment.AttachmentIndex].HasResolve ? m_AttachmentData[attachment.AttachmentIndex].BlitImage : m_AttachmentData[attachment.AttachmentIndex].Image);
    }

    void* OpenGLFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Color attachment access on framebuffer out of range");

        if (m_Info.SampleCount == MsaaSampleCount::None)
            return (void*)static_cast<size_t>(m_AttachmentData[attachmentIndex].Image);
        else
            return (void*)static_cast<size_t>(m_AttachmentData[attachmentIndex].BlitImage);
    }

    void* OpenGLFramebuffer::GetDepthAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_DepthAttachmentData.size(), "Depth attachment access on framebuffer out of range");
        HE_ENGINE_ASSERT(m_Info.SampleCount == MsaaSampleCount::None, "Cannot get framebuffer depth attachment handle, SampleCount != None");

        if (m_Info.SampleCount == MsaaSampleCount::None)
            return (void*)static_cast<size_t>(m_DepthAttachmentData[attachmentIndex].Image);
        else
            return (void*)static_cast<size_t>(m_DepthAttachmentData[attachmentIndex].BlitImage);
    }

    void* OpenGLFramebuffer::GetColorAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_PixelBufferMappings.size(), "Attachment of pixel read out of range");
        HE_ENGINE_ASSERT(m_Info.ColorAttachments[attachmentIndex].AllowCPURead, "Cannot read pixel data of attachment that does not have 'AllowCPURead' enabled");

        if (m_PixelBufferMappings[attachmentIndex] == nullptr)
            m_PixelBufferMappings[attachmentIndex] = glMapNamedBuffer(m_PixelBufferObjects[attachmentIndex][(App::Get().GetFrameCount() + 1) % 2]->GetBufferId(), GL_READ_ONLY);

        return m_PixelBufferMappings[attachmentIndex];
    }

    void* OpenGLFramebuffer::GetDepthAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_PixelBufferMappings.size(), "Attachment of pixel read out of range");
        HE_ENGINE_ASSERT(m_Info.ColorAttachments[attachmentIndex].AllowCPURead, "Cannot read pixel data of attachment that does not have 'AllowCPURead' enabled");

        if (m_PixelBufferMappings[attachmentIndex] == nullptr)
            m_PixelBufferMappings[attachmentIndex] = glMapNamedBuffer(m_PixelBufferObjects[attachmentIndex][(App::Get().GetFrameCount() + 1) % 2]->GetBufferId(), GL_READ_ONLY);

        return m_PixelBufferMappings[attachmentIndex];
    }

    void OpenGLFramebuffer::ClearOutputAttachment(u32 outputAttachmentIndex, bool clearDepth)
    {
        u32 attachmentIndex = 0;
        for (size_t i = 0; i < m_Info.Subpasses[m_CurrentSubpass].OutputAttachments.size(); i++)
        {
            if (m_Info.Subpasses[m_CurrentSubpass].OutputAttachments[i].Type == SubpassAttachmentType::Color)
                attachmentIndex++;
            if (attachmentIndex == outputAttachmentIndex)
            {
                attachmentIndex = m_Info.Subpasses[m_CurrentSubpass].OutputAttachments[i].AttachmentIndex;
                break;
            }
        }

        auto& attachment = m_AttachmentData[attachmentIndex];
        glClearTexImage(attachment.Image, 0, attachment.ColorFormat, GL_FLOAT, &m_Info.ColorAttachments[attachmentIndex].ClearColor);

        if (clearDepth)
        {
            if (Renderer::IsUsingReverseDepth())
                glClearDepth(0.f);
            else
                glClearDepth(1.f);
            glClear(GL_DEPTH_BUFFER_BIT);
            glClear(GL_STENCIL_BUFFER_BIT);
        }
    }

    void OpenGLFramebuffer::StartNextSubpass()
    {
        m_CurrentSubpass++;

        // if using a multisampled buffer, perform a framebuffer blit (copy) operation on the previous subpass
        if (m_CurrentSubpass > 0)
            BlitFramebuffers(m_CurrentSubpass - 1);

        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[m_CurrentSubpass]);
        glViewport(0, 0, m_ActualWidth, m_ActualHeight);

        // enable draw for all attachments
        glDrawBuffers(GetSubpassOutputColorAttachmentCount(m_CurrentSubpass), m_CachedAttachmentHandles.data());
    }

    Ref<GraphicsPipeline> OpenGLFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == GetSubpassOutputColorAttachmentCount(createInfo.SubpassIndex), "Graphics pipeline blend state count must match subpass color attachment output count");

        return CreateRef<OpenGLGraphicsPipeline>(createInfo);
    }

    void OpenGLFramebuffer::CreateAttachmentTextures(OpenGLFramebufferAttachment& attachment)
    {
        int mainTextureTarget = attachment.HasResolve ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;
        glCreateTextures(mainTextureTarget, 1, &attachment.Image);

        glBindTexture(mainTextureTarget, attachment.Image);

        if (attachment.HasResolve)
            glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, m_ImageSamples, attachment.ColorFormatInternal, m_ActualWidth, m_ActualHeight, GL_FALSE);
        else
        {
            if (attachment.IsDepthAttachment)
                glTexStorage2D(GL_TEXTURE_2D, 1, attachment.ColorFormatInternal, m_ActualWidth, m_ActualHeight);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, attachment.ColorFormatInternal, m_ActualWidth, m_ActualHeight, 0, attachment.ColorFormat, GL_UNSIGNED_BYTE, NULL);
            

            // TODO: dynamic filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }

        if (attachment.HasResolve)
        {
            glCreateTextures(GL_TEXTURE_2D, 1, &attachment.BlitImage);

            glBindTexture(GL_TEXTURE_2D, attachment.BlitImage);

            if (attachment.IsDepthAttachment)
                glTexStorage2D(GL_TEXTURE_2D, 1, attachment.ColorFormatInternal, m_ActualWidth, m_ActualHeight);
            else
                glTexImage2D(GL_TEXTURE_2D, 0, attachment.ColorFormatInternal, m_ActualWidth, m_ActualHeight, 0, attachment.ColorFormat, GL_UNSIGNED_BYTE, NULL);

            // TODO: dynamic filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        }
    }

    void OpenGLFramebuffer::CleanupAttachmentTextures(OpenGLFramebufferAttachment& attachment)
    {
        glDeleteTextures(1, &attachment.Image);
        if (attachment.HasResolve)
            glDeleteTextures(1, &attachment.BlitImage);
    }

    void OpenGLFramebuffer::CreateFramebuffers()
    {
        glGenFramebuffers(static_cast<int>(m_Framebuffers.size()), m_Framebuffers.data());
        if (m_ImageSamples > 1)
            glGenFramebuffers(static_cast<int>(m_BlitFramebuffers.size()), m_BlitFramebuffers.data());

        for (size_t i = 0; i < m_Info.Subpasses.size(); i++)
        {
            bool depthFree = true;
            u32 attachmentIndex = 0;
            for (auto& output : m_Info.Subpasses[i].OutputAttachments)
            {
                if (output.Type == SubpassAttachmentType::Depth)
                {
                    HE_ENGINE_ASSERT(depthFree, "Cannot bind more than one depth attachment to the output of a subpass");
                    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, m_ImageSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,  m_DepthAttachmentData[output.AttachmentIndex].Image, 0);
                    if (m_DepthAttachmentData[output.AttachmentIndex].HasResolve)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, m_BlitFramebuffers[i]);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D,  m_DepthAttachmentData[output.AttachmentIndex].BlitImage, 0);
                    }
                    depthFree = false;
                }
                else
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex, m_ImageSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,  m_AttachmentData[output.AttachmentIndex].Image, 0);
                    if (m_AttachmentData[output.AttachmentIndex].HasResolve)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, m_BlitFramebuffers[i]);
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + attachmentIndex, GL_TEXTURE_2D,  m_AttachmentData[output.AttachmentIndex].BlitImage, 0);
                    }
                    attachmentIndex++;
                }
            }

            if (glCheckNamedFramebufferStatus(m_Framebuffers[i], GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {
                HE_ENGINE_LOG_CRITICAL("Failed to create OpenGL framebuffer: {0}", glCheckFramebufferStatus(GL_FRAMEBUFFER));
                HE_ENGINE_ASSERT(false);
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    void OpenGLFramebuffer::CleanupFramebuffers()
    {
        glDeleteFramebuffers(static_cast<int>(m_Framebuffers.size()), m_Framebuffers.data());
        if (m_ImageSamples > 1)
            glDeleteFramebuffers(static_cast<int>(m_BlitFramebuffers.size()), m_BlitFramebuffers.data());
    }

    void OpenGLFramebuffer::CreatePixelBuffers()
    {
        for (size_t i = 0; i < m_Info.ColorAttachments.size(); i++)
        {
            if (m_Info.ColorAttachments[i].AllowCPURead) // only create the buffers for the no sample count textures
            {
                for (size_t j = 0; j < m_PixelBufferObjects[0].size(); j++)
                {
                    m_PixelBufferObjects[i][j] = std::dynamic_pointer_cast<OpenGLBuffer>(Buffer::Create(
                        Buffer::Type::Pixel,
                        BufferUsageType::Dynamic,
                        { ColorFormatBufferDataType(m_Info.ColorAttachments[i].Format) },
                        m_ActualWidth * m_ActualHeight * ColorFormatComponents(m_Info.ColorAttachments[i].Format)
                    ));
                }
            }
        }
    }

    void OpenGLFramebuffer::CleanupPixelBuffers()
    {
        for (size_t i = 0; i < m_Info.ColorAttachments.size(); i++)
        {
            if (m_Info.ColorAttachments[i].AllowCPURead)
            {
                for (size_t j = 0; j < m_PixelBufferObjects[0].size(); j++)
                {
                    m_PixelBufferObjects[i][j].reset();
                }
            }
        }
    }

    void OpenGLFramebuffer::Recreate()
    {
        CleanupFramebuffers();

        for (auto& attachmentData : m_DepthAttachmentData)
        {
            CleanupAttachmentTextures(attachmentData);
            CreateAttachmentTextures(attachmentData);
        }
        for (auto& attachmentData : m_AttachmentData)
        {
            CleanupAttachmentTextures(attachmentData);
            CreateAttachmentTextures(attachmentData);
        }

        CreateFramebuffers();

        m_Valid = true;
    }

    void OpenGLFramebuffer::BlitFramebuffers(int subpassIndex)
    {
        if (m_ImageSamples > 1)
        {
            glBindFramebuffer(GL_READ_FRAMEBUFFER, m_Framebuffers[subpassIndex]);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_BlitFramebuffers[subpassIndex]);

            // copy all attachments
            for (size_t i = 0; i < GetSubpassOutputColorAttachmentCount(subpassIndex); i++)
            {
                glReadBuffer(m_CachedAttachmentHandles[i]);
                glDrawBuffer(m_CachedAttachmentHandles[i]);
                glBlitFramebuffer(0, 0, m_ActualWidth, m_ActualHeight, 0, 0, m_ActualWidth, m_ActualHeight, GL_COLOR_BUFFER_BIT | (HasOutputDepthAttachment(subpassIndex) ? GL_DEPTH_BUFFER_BIT : 0), GL_NEAREST);
            }
        }
        
    }
}
