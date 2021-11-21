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
        
        glGenTextures(1, &m_TextureId);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);

        float maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);

        // TODO: paramaterize
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[0]));	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[1]));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWWrap[2]));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGLCommon::SamplerFilterToOpenGLWithMipmap(m_SamplerState.MinFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGLCommon::SamplerFilterToOpenGL(m_SamplerState.MagFilter));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, m_SamplerState.AnisotropyEnable ? std::min(maxAnisotropy, static_cast<float>(m_SamplerState.MaxAnisotropy)) : 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, createInfo.Width, createInfo.Height, 0, GL_RGBA, createInfo.FloatComponents ? GL_FLOAT : GL_UNSIGNED_BYTE, initialData);
        glGenerateMipmap(GL_TEXTURE_2D);

        //m_ImGuiHandle = (void*)static_cast<size_t>(m_TextureId);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        glDeleteTextures(1, &m_TextureId);
    }
}