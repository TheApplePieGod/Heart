#include "hepch.h"
#include "OpenGLTexture.h"

#include "Heart/Core/App.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "glad/glad.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    OpenGLTexture::OpenGLTexture(const TextureCreateInfo& createInfo, void* initialData)
        : Texture(createInfo)
    {
        if (initialData != nullptr)
            ScanForTransparency(createInfo.Width, createInfo.Height, createInfo.Channels, initialData);
        
        // opengl will generate more, but we will pretend like they don't exist ;)
        m_MipLevels = m_Info.MipCount;
        u32 maxMipLevels = static_cast<u32>(floor(log2(std::max(m_Info.Width, m_Info.Height)))) + 1;
        if (m_MipLevels > maxMipLevels || m_MipLevels == 0)
            m_MipLevels = maxMipLevels;

        int type = OpenGLCommon::BufferDataTypeToBaseOpenGL(createInfo.DataType);
        m_GeneralFormat = BufferDataTypeColorFormat(createInfo.DataType, m_Info.Channels);
        m_Format = OpenGLCommon::ColorFormatToOpenGL(m_GeneralFormat);
        m_InternalFormat = OpenGLCommon::ColorFormatToInternalOpenGL(m_GeneralFormat);

        // if the texture is cpu visible, create the readonly buffer
        if (createInfo.AllowCPURead)
        {
            for (size_t i = 0; i < m_PixelBuffers.size(); i++)
            {
                m_PixelBuffers[i] = std::dynamic_pointer_cast<OpenGLBuffer>(Buffer::Create(
                    Buffer::Type::Pixel,
                    BufferUsageType::Dynamic,
                    { m_Info.DataType },
                    m_Info.Width * m_Info.Height * m_Info.Channels,
                    initialData
                ));
            }
        }

        m_Target = GL_TEXTURE_2D;
        if (m_Info.ArrayCount > 1)
            m_Target = GL_TEXTURE_2D_ARRAY;
        if (m_Info.ArrayCount == 6)
            m_Target = GL_TEXTURE_CUBE_MAP;

        glGenTextures(1, &m_TextureId);
        glBindTexture(m_Target, m_TextureId);

        float maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);

        // apply sampler params to the main texture
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[0]));	
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[1]));
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[2]));
        glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, OpenGLCommon::SamplerFilterToOpenGLWithMipmap(m_Info.SamplerState.MinFilter));
        glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, OpenGLCommon::SamplerFilterToOpenGL(m_Info.SamplerState.MagFilter));
        glTexParameteri(m_Target, GL_TEXTURE_REDUCTION_MODE_ARB, OpenGLCommon::SamplerReductionModeToOpenGL(m_Info.SamplerState.ReductionMode));
        glTexParameterf(m_Target, GL_TEXTURE_MAX_ANISOTROPY, m_Info.SamplerState.AnisotropyEnable ? std::min(maxAnisotropy, static_cast<float>(m_Info.SamplerState.MaxAnisotropy)) : 1);

        if (m_Info.ArrayCount == 1)
        {
            glTexStorage2D(GL_TEXTURE_2D, m_MipLevels, m_InternalFormat, m_Info.Width, m_Info.Height);
            if (initialData != nullptr)
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, m_Info.Width, m_Info.Height, m_Format, type, initialData);
        }
        else if (m_Info.ArrayCount == 6) // cubemap
        {
            glTexStorage2D(GL_TEXTURE_CUBE_MAP, m_MipLevels, m_InternalFormat, m_Info.Width, m_Info.Height);
            if (initialData != nullptr)
                glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, 0, 0, m_Info.Width, m_Info.Height, m_Format, type, initialData);
        }
        else
        {
            glTexStorage3D(GL_TEXTURE_2D_ARRAY, m_MipLevels, m_InternalFormat, m_Info.Width, m_Info.Height, m_Info.ArrayCount);
            if (initialData != nullptr)
                glTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, 0, m_Info.Width, m_Info.Height, 1, m_Format, type, initialData);
        }

        glGenerateMipmap(m_Target);

        // generate a texture view for each layer / mipmap
        m_ViewTextures.resize(m_Info.ArrayCount * m_MipLevels);
        glGenTextures(static_cast<u32>(m_ViewTextures.size()), m_ViewTextures.data());
        u32 viewIndex = 0;
        for (u32 i = 0; i < m_Info.ArrayCount; i++)
        {
            for (u32 j = 0; j < m_MipLevels; j++)
            {
                glTextureView(m_ViewTextures[viewIndex], GL_TEXTURE_2D, m_TextureId, m_InternalFormat, j, 1, i, 1);

                // apply the sampler params to each view because they do not inherit
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_WRAP_S, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[0]));	
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_WRAP_T, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[1]));
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_WRAP_R, OpenGLCommon::SamplerWrapModeToOpenGL(m_Info.SamplerState.UVWWrap[2]));
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_MIN_FILTER, OpenGLCommon::SamplerFilterToOpenGLWithMipmap(m_Info.SamplerState.MinFilter));
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_MAG_FILTER, OpenGLCommon::SamplerFilterToOpenGL(m_Info.SamplerState.MagFilter));
                glTextureParameteri(m_ViewTextures[viewIndex], GL_TEXTURE_REDUCTION_MODE_ARB, OpenGLCommon::SamplerReductionModeToOpenGL(m_Info.SamplerState.ReductionMode));
                glTextureParameterf(m_ViewTextures[viewIndex], GL_TEXTURE_MAX_ANISOTROPY, m_Info.SamplerState.AnisotropyEnable ? std::min(maxAnisotropy, static_cast<float>(m_Info.SamplerState.MaxAnisotropy)) : 1);

                m_ImGuiHandles.emplace_back((void*)static_cast<size_t>(m_ViewTextures[viewIndex]));
                viewIndex++;
            }
        }

        glBindTexture(m_Target, 0);
    }

    void OpenGLTexture::RegenerateMipMaps()
    {
        glBindTexture(m_Target, m_TextureId);
        glGenerateMipmap(m_Target);
        glBindTexture(m_Target, 0);
    }

    void* OpenGLTexture::GetPixelData()
    {
        HE_ENGINE_ASSERT(m_Info.AllowCPURead, "Cannot read pixel data of texture that does not have 'AllowCPURead' enabled");

        // Any unmapping will be handled in the framebuffer that draws to this image if applicable
        return m_PixelBuffers[(App::Get().GetFrameCount() + 1) % 2]->Map(true);
    }

    void* OpenGLTexture::GetImGuiHandle(u32 layerIndex, u32 mipLevel)
    {
        return m_ImGuiHandles[layerIndex * m_MipLevels + mipLevel];
    }

    OpenGLTexture::~OpenGLTexture()
    {
        if (m_Info.AllowCPURead)
            for (auto& buffer : m_PixelBuffers)
                buffer->Unmap();

        glDeleteTextures(1, &m_TextureId);
    }
}