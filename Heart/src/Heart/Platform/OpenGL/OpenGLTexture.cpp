#include "htpch.h"
#include "OpenGLTexture.h"

#include "glad/glad.h"
#include "stb_image/stb_image.h"

namespace Heart
{
    OpenGLTexture::OpenGLTexture(const std::string& path, int width, int height, int channels, void* data)
        : Texture(path, width, height, channels)
    {
        bool load = data == nullptr;
        if (load)
        {
            data = stbi_load(path.c_str(), &m_Width, &m_Height, &m_Channels, m_DesiredChannelCount);
            if (data == nullptr)
            {
                HE_ENGINE_LOG_ERROR("Failed to load image at path {0}", path);
                HE_ENGINE_ASSERT(false);
            }
            HE_ENGINE_LOG_TRACE("Texture info: {0}x{1} w/ {2} channels", m_Width, m_Height, m_Channels);
        }
        
        ScanForTransparency(width, height, channels, data);
        
        glGenTextures(1, &m_TextureId);
        glBindTexture(GL_TEXTURE_2D, m_TextureId);

        // TODO: paramaterize
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, m_Width, m_Height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        //glGenerateMipmap(GL_TEXTURE_2D);

        if (load)
            stbi_image_free(data);

        m_ImGuiHandle = (void*)static_cast<size_t>(m_TextureId);
    }

    OpenGLTexture::~OpenGLTexture()
    {
        glDeleteTextures(1, &m_TextureId);
    }
}