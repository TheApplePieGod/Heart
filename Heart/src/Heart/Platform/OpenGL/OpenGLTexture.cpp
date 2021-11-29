#include "htpch.h"
#include "OpenGLTexture.h"

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

        int type = m_Info.FloatComponents ? GL_FLOAT : GL_UNSIGNED_BYTE;
        m_Format = GL_RGBA;
        m_InternalFormat = m_Info.FloatComponents ? GL_RGBA32F : GL_RGBA8;
        m_GeneralFormat = m_Info.FloatComponents ? ColorFormat::RGBA32F : ColorFormat::RGBA8;

        m_Target = GL_TEXTURE_2D;
        if (m_Info.ArrayCount > 1)
            m_Target = GL_TEXTURE_2D_ARRAY;
        if (m_Info.ArrayCount == 6)
            m_Target = GL_TEXTURE_CUBE_MAP;

        glGenTextures(1, &m_TextureId);
        glBindTexture(m_Target, m_TextureId);

        float maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);

        // TODO: paramaterize
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_S, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[0]));	
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_T, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[1]));
        glTexParameteri(m_Target, GL_TEXTURE_WRAP_R, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[2]));
        glTexParameteri(m_Target, GL_TEXTURE_MIN_FILTER, OpenGLCommon::SamplerFilterToOpenGLWithMipmap(m_SamplerState.MinFilter));
        glTexParameteri(m_Target, GL_TEXTURE_MAG_FILTER, OpenGLCommon::SamplerFilterToOpenGL(m_SamplerState.MagFilter));
        glTexParameterf(m_Target, GL_TEXTURE_MAX_ANISOTROPY, m_SamplerState.AnisotropyEnable ? std::min(maxAnisotropy, static_cast<float>(m_SamplerState.MaxAnisotropy)) : 1);

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
                if (j == 0)
                    m_LayerImGuiHandles.emplace_back((void*)static_cast<size_t>(m_ViewTextures[viewIndex]));
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

    OpenGLTexture::~OpenGLTexture()
    {
        glDeleteTextures(1, &m_TextureId);
    }
}