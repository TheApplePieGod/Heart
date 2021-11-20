#include "htpch.h"
#include "OpenGLTexture.h"

#include "Heart/Platform/OpenGL/OpenGLCommon.h"
#include "glad/glad.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    OpenGLTexture::OpenGLTexture(const std::string& path, bool floatComponents, int width, int height, int channels, void* data)
        : Texture(path, floatComponents, width, height, channels)
    {
        bool load = data == nullptr;
        if (load)
        {
            if (floatComponents)
                data = stbi_loadf(path.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
            else
                data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
            if (data == nullptr)
            {
                HE_ENGINE_LOG_ERROR("Failed to load image at path {0}", path);
                HE_ENGINE_ASSERT(false);
            }
            HE_ENGINE_LOG_TRACE("Texture info: {0}x{1} w/ {2} channels, float components: {3}", m_Width, m_Height, m_Channels, floatComponents);
        }
        
        ScanForTransparency(width, height, channels, data);
        
        glGenTextures(1, &m_TextureId);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);

        float maxAnisotropy;
        glGetFloatv(GL_MAX_TEXTURE_MAX_ANISOTROPY, &maxAnisotropy);

        // TODO: paramaterize
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWrap[0]));	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, OpenGLCommon::SamplerWrapModeToOpenGL(m_SamplerState.UVWrap[1]));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, OpenGLCommon::SamplerFilterToOpenGLWithMipmap(m_SamplerState.MinFilter));
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, OpenGLCommon::SamplerFilterToOpenGL(m_SamplerState.MagFilter));
        glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, m_SamplerState.AnisotropyEnable ? std::min(maxAnisotropy, static_cast<float>(m_SamplerState.MaxAnisotropy)) : 1);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, floatComponents ? GL_FLOAT : GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        if (load)
            stbi_image_free(data);

        m_ImGuiHandle = (void*)static_cast<size_t>(m_TextureId);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        glDeleteTextures(1, &m_TextureId);
    }
}