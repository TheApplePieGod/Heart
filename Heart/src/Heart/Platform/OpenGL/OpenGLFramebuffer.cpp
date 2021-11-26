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
        
        m_ImageSamples = OpenGLCommon::MsaaSampleCountToOpenGL(createInfo.SampleCount);
        if (m_ImageSamples > OpenGLContext::MaxMsaaSamples())
            m_ImageSamples = OpenGLContext::MaxMsaaSamples();

        for (auto& attachment : m_Info.DepthAttachments)
        {
            OpenGLFramebufferAttachment attachmentData{};
            attachmentData.GeneralColorFormat = m_GeneralDepthFormat;
            attachmentData.ColorFormat = m_DepthFormat;
            attachmentData.ColorFormatInternal = m_DepthFormatInternal;
            attachmentData.HasResolve = m_ImageSamples > 1; // createInfo.SampleCount != MsaaSampleCount::None;
            attachmentData.CPUVisible = false;
            attachmentData.IsDepthAttachment = true;

            CreateAttachmentTextures(attachmentData);

            m_DepthAttachmentData.emplace_back(attachmentData);
        }

        for (auto& attachment : m_Info.ColorAttachments)
        {
            OpenGLFramebufferAttachment attachmentData{};
            if (attachment.Texture)
            {
                attachmentData.ExternalTexture = (OpenGLTexture*)attachment.Texture.get();
                attachmentData.ExternalTextureLayer = attachment.LayerIndex;
                attachmentData.ExternalTextureMip = attachment.MipLevel;

                HE_ENGINE_ASSERT(m_Info.Width == attachmentData.ExternalTexture->GetMipWidth(attachment.MipLevel), "Texture dimensions (at the specified mip level) must match the framebuffer (framebuffer width/height cannot be zero)");
                HE_ENGINE_ASSERT(m_Info.Height == attachmentData.ExternalTexture->GetMipHeight(attachment.MipLevel), "Texture dimensions (at the specified mip level) must match the framebuffer (framebuffer width/height cannot be zero)");
            }

            attachmentData.GeneralColorFormat = attachment.Texture ? attachmentData.ExternalTexture->GetGeneralFormat() : attachment.Format;
            attachmentData.ColorFormat = attachment.Texture ? attachmentData.ExternalTexture->GetFormat() : OpenGLCommon::ColorFormatToOpenGL(attachment.Format);
            attachmentData.ColorFormatInternal = attachment.Texture ? attachmentData.ExternalTexture->GetInternalFormat() : OpenGLCommon::ColorFormatToInternalOpenGL(attachment.Format);
            attachmentData.HasResolve = m_ImageSamples > 1;
            attachmentData.CPUVisible = attachment.AllowCPURead;
            attachmentData.IsDepthAttachment = false;

            CreateAttachmentTextures(attachmentData);

            CreatePixelBuffers(attachmentData);

            m_AttachmentData.emplace_back(attachmentData);
        }

        m_Framebuffers.resize(createInfo.Subpasses.size());
        if (m_ImageSamples > 1)
            m_BlitFramebuffers.resize(createInfo.Subpasses.size());

        CreateFramebuffers();
    }

    OpenGLFramebuffer::~OpenGLFramebuffer()
    {
        CleanupFramebuffers();

        for (auto& attachmentData : m_AttachmentData)
        {
            CleanupAttachmentTextures(attachmentData);
            CleanupPixelBuffers(attachmentData);
        }
        for (auto& attachmentData : m_DepthAttachmentData)
            CleanupAttachmentTextures(attachmentData);
    }

    void OpenGLFramebuffer::Bind()
    {
        HE_PROFILE_FUNCTION();

        if (!m_Valid)
            Recreate();

        if (m_CurrentSubpass == -1)
        {
            // clear each attachment with the provided color
            for (size_t i = 0; i < m_AttachmentData.size(); i++)
                glClearTexImage(m_AttachmentData[i].Image, 0, m_AttachmentData[i].ColorFormat, GL_FLOAT, &m_Info.ColorAttachments[i].ClearColor);

            // sorta jank
            // in order to clear all the depth buffers we will bind every framebuffer and clear them through there
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

            StartNextSubpass();
        }

        OpenGLContext::SetBoundFramebuffer(this);
    }

    void OpenGLFramebuffer::Submit()
    {
        HE_ENGINE_ASSERT(m_CurrentSubpass == m_Info.Subpasses.size() - 1, "Attempting to submit a framebuffer without completing all subpasses");

        BlitFramebuffers(m_CurrentSubpass);

        for (auto& attachment : m_AttachmentData)
            PopulatePixelBuffer(attachment);

        // int error = glGetError(); 
        // if (error != 0)
        // {
        //     HE_ENGINE_LOG_ERROR("ERROR {0}", error);
        //     HE_ENGINE_ASSERT(false);
        // }

        m_CurrentSubpass = -1;
        m_FlushedThisFrame = false;
        m_BoundPipeline = nullptr;
        m_BoundPipelineName = "";
    }

    void OpenGLFramebuffer::BindPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION();
        
        if (name == m_BoundPipelineName) return;

        auto pipeline = static_cast<OpenGLGraphicsPipeline*>(LoadPipeline(name).get());
        glUseProgram(pipeline->GetProgramId());
        glBindVertexArray(pipeline->GetVertexArrayId());

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

        m_BoundPipeline = pipeline;
        m_BoundPipelineName = name;
    }

    void OpenGLFramebuffer::BindShaderBufferResource(u32 bindingIndex, u32 elementOffset, Buffer* _buffer)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline before BindShaderResource");

        OpenGLBuffer& buffer = static_cast<OpenGLBuffer&>(*_buffer);

        glBindBufferBase(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId());
        glBindBufferRange(OpenGLCommon::BufferTypeToOpenGL(buffer.GetType()), bindingIndex, buffer.GetBufferId(), elementOffset * buffer.GetLayout().GetStride(), buffer.GetAllocatedSize());
    }

    void OpenGLFramebuffer::BindShaderTextureResource(u32 bindingIndex, Texture* _texture)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline before BindShaderResource");

        OpenGLTexture& texture = static_cast<OpenGLTexture&>(*_texture);

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        glBindTexture(texture.GetTarget(), texture.GetTextureId());
    }

    void OpenGLFramebuffer::BindShaderTextureLayerResource(u32 bindingIndex, Texture* _texture, u32 layerIndex)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline before BindShaderResource");

        OpenGLTexture& texture = static_cast<OpenGLTexture&>(*_texture);

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        glBindTexture(GL_TEXTURE_2D, texture.GetLayerTextureId(layerIndex, 0));
    }

    void OpenGLFramebuffer::BindSubpassInputAttachment(u32 bindingIndex, SubpassAttachment attachment)
    {
        HE_PROFILE_FUNCTION();
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline before BindShaderResource");

        glActiveTexture(GL_TEXTURE0 + bindingIndex);
        if (attachment.Type == SubpassAttachmentType::Depth)
            glBindTexture(GL_TEXTURE_2D, m_DepthAttachmentData[attachment.AttachmentIndex].HasResolve ? m_DepthAttachmentData[attachment.AttachmentIndex].BlitImage : m_DepthAttachmentData[attachment.AttachmentIndex].Image);
        else
            glBindTexture(GL_TEXTURE_2D, m_AttachmentData[attachment.AttachmentIndex].HasResolve ? m_AttachmentData[attachment.AttachmentIndex].BlitImage : m_AttachmentData[attachment.AttachmentIndex].Image);
    }

    void OpenGLFramebuffer::FlushBindings()
    {
        HE_ENGINE_ASSERT(m_BoundPipeline != nullptr, "Must call BindPipeline and bind all resources before FlushBindings");

        m_FlushedThisFrame = true;
    }

    void* OpenGLFramebuffer::GetColorAttachmentImGuiHandle(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Color attachment access on framebuffer out of range");

        if (m_Info.SampleCount == MsaaSampleCount::None)
            return (void*)static_cast<size_t>(m_AttachmentData[attachmentIndex].Image);
        else
            return (void*)static_cast<size_t>(m_AttachmentData[attachmentIndex].BlitImage);
    }

    void* OpenGLFramebuffer::GetColorAttachmentPixelData(u32 attachmentIndex)
    {
        HE_ENGINE_ASSERT(attachmentIndex < m_AttachmentData.size(), "Color attachment of pixel read out of range");
        HE_ENGINE_ASSERT(m_AttachmentData[attachmentIndex].CPUVisible, "Cannot read pixel data of attachment that does not have 'AllowCPURead' enabled");

        if (m_AttachmentData[attachmentIndex].PixelBufferMapping == nullptr)
            m_AttachmentData[attachmentIndex].PixelBufferMapping = glMapNamedBuffer(m_AttachmentData[attachmentIndex].PixelBuffers[(App::Get().GetFrameCount() + 1) % 2]->GetBufferId(), GL_READ_ONLY);

        return m_AttachmentData[attachmentIndex].PixelBufferMapping;
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

        HE_ENGINE_ASSERT(m_CurrentSubpass < m_Info.Subpasses.size(), "Attempting to start a subpass that does not exist");

        // if using a multisampled buffer, perform a framebuffer blit (copy) operation on the previous subpass
        if (m_CurrentSubpass > 0)
            BlitFramebuffers(m_CurrentSubpass - 1);

        glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[m_CurrentSubpass]);
        glViewport(0, 0, m_ActualWidth, m_ActualHeight);

        // enable draw for all attachments
        glDrawBuffers(GetSubpassOutputColorAttachmentCount(m_CurrentSubpass), m_CachedAttachmentHandles.data());

        m_FlushedThisFrame = false;
        m_BoundPipeline = nullptr;
        m_BoundPipelineName = "";
    }

    Ref<GraphicsPipeline> OpenGLFramebuffer::InternalInitializeGraphicsPipeline(const GraphicsPipelineCreateInfo& createInfo)
    {
        HE_ENGINE_ASSERT(createInfo.BlendStates.size() == GetSubpassOutputColorAttachmentCount(createInfo.SubpassIndex), "Graphics pipeline blend state count must match subpass color attachment output count");

        return CreateRef<OpenGLGraphicsPipeline>(createInfo);
    }

    void OpenGLFramebuffer::CreateAttachmentTextures(OpenGLFramebufferAttachment& attachment)
    {
        int mainTextureTarget = attachment.HasResolve ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D;

        // create the associated images for the framebuffer
        // if we are using an external texture, that texture will be the resolve image in the case of a multisampled framebuffer
        // for one sample, we will render directly to that image

        bool shouldResolveExternal = attachment.ExternalTexture && attachment.HasResolve;
        bool shouldCreateBaseTexture = !attachment.ExternalTexture || shouldResolveExternal;
        bool shouldCreateResolveTexture = attachment.HasResolve && !attachment.ExternalTexture;

        if (shouldCreateBaseTexture)
        {
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
        }

        if (shouldCreateResolveTexture)
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

        if (shouldResolveExternal)
            attachment.BlitImage = attachment.ExternalTexture->GetLayerTextureId(attachment.ExternalTextureLayer, attachment.ExternalTextureMip);
        else if (attachment.ExternalTexture)
            attachment.Image = attachment.ExternalTexture->GetLayerTextureId(attachment.ExternalTextureLayer, attachment.ExternalTextureMip);
    }

    void OpenGLFramebuffer::CleanupAttachmentTextures(OpenGLFramebufferAttachment& attachment)
    {
        bool shouldResolveExternal = attachment.ExternalTexture && attachment.HasResolve;
        bool hasBaseTexture = !attachment.ExternalTexture || shouldResolveExternal;
        bool hasResolveTexture = attachment.HasResolve && !attachment.ExternalTexture;

        if (hasBaseTexture)
            glDeleteTextures(1, &attachment.Image);
        if (hasResolveTexture)
            glDeleteTextures(1, &attachment.BlitImage);
    }

    void OpenGLFramebuffer::PopulatePixelBuffer(OpenGLFramebufferAttachment& attachment)
    {
        if (!attachment.CPUVisible) return;

        // unmap any buffers that were mapped this frame
        if (attachment.PixelBufferMapping != nullptr)
        {
            glUnmapNamedBuffer(attachment.PixelBuffers[(App::Get().GetFrameCount() + 1) % 2]->GetBufferId());
            attachment.PixelBufferMapping = nullptr;
        }

        // start the async read for cpu visible attachments
        glBindFramebuffer(GL_READ_FRAMEBUFFER, m_PBOFramebuffer);
        glReadBuffer(m_CachedAttachmentHandles[attachment.PBOFramebufferAttachment]);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, attachment.PixelBuffers[App::Get().GetFrameCount() % 2]->GetBufferId());
        glReadPixels(0, 0, m_ActualWidth, m_ActualHeight, attachment.ColorFormat, OpenGLCommon::ColorFormatToOpenGLDataType(attachment.GeneralColorFormat), nullptr);
        glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
    }

    void OpenGLFramebuffer::CreateFramebuffers()
    {
        glGenFramebuffers(static_cast<int>(m_Framebuffers.size()), m_Framebuffers.data());
        if (m_ImageSamples > 1)
            glGenFramebuffers(static_cast<int>(m_BlitFramebuffers.size()), m_BlitFramebuffers.data());

        // create a framebuffer which we bind all CPU visible attachments to so we can use glReadPixels at the end of the frame
        u32 pboAttachmentIndex = 0;
        glGenFramebuffers(1, &m_PBOFramebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, m_PBOFramebuffer);
        for (auto& attachment : m_AttachmentData)
        {
            if (attachment.CPUVisible)
            {
                if (attachment.HasResolve)
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + pboAttachmentIndex, GL_TEXTURE_2D,  attachment.BlitImage, 0);
                else
                    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + pboAttachmentIndex, GL_TEXTURE_2D,  attachment.Image, 0);
                attachment.PBOFramebufferAttachment = pboAttachmentIndex;
                pboAttachmentIndex++;
            }
        }

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
                        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_TEXTURE_2D, m_DepthAttachmentData[output.AttachmentIndex].BlitImage, 0);
                    }
                    depthFree = false;
                }
                else
                {
                    glBindFramebuffer(GL_FRAMEBUFFER, m_Framebuffers[i]);
                    glFramebufferTexture2D(GL_FRAMEBUFFER, m_CachedAttachmentHandles[attachmentIndex], m_ImageSamples > 1 ? GL_TEXTURE_2D_MULTISAMPLE : GL_TEXTURE_2D,  m_AttachmentData[output.AttachmentIndex].Image, 0);
                    if (m_AttachmentData[output.AttachmentIndex].HasResolve)
                    {
                        glBindFramebuffer(GL_FRAMEBUFFER, m_BlitFramebuffers[i]);
                        glFramebufferTexture2D(GL_FRAMEBUFFER,  m_CachedAttachmentHandles[attachmentIndex], GL_TEXTURE_2D,  m_AttachmentData[output.AttachmentIndex].BlitImage, 0);
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

        glDeleteFramebuffers(1, &m_PBOFramebuffer);
    }

    void OpenGLFramebuffer::CreatePixelBuffers(OpenGLFramebufferAttachment& attachment)
    {
        for (auto& attachment : m_AttachmentData)
        {
            if (attachment.CPUVisible)
            {
                for (size_t i = 0; i < attachment.PixelBuffers.size(); i++)
                {
                    attachment.PixelBuffers[i] = std::dynamic_pointer_cast<OpenGLBuffer>(Buffer::Create(
                        Buffer::Type::Pixel,
                        BufferUsageType::Dynamic,
                        { ColorFormatBufferDataType(attachment.GeneralColorFormat) },
                        m_ActualWidth * m_ActualHeight * ColorFormatComponents(attachment.GeneralColorFormat)
                    ));
                }
            }
        }
    }

    void OpenGLFramebuffer::CleanupPixelBuffers(OpenGLFramebufferAttachment& attachment)
    {
        for (auto& attachment : m_AttachmentData)
        {
            if (attachment.CPUVisible)
            {
                for (auto& buffer : attachment.PixelBuffers)
                    buffer.reset();
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
            CleanupPixelBuffers(attachmentData);
            CleanupAttachmentTextures(attachmentData);
            CreateAttachmentTextures(attachmentData);
            CreatePixelBuffers(attachmentData);
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
