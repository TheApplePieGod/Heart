#include "htpch.h"
#include "OpenGLTexture.h"

#include "stb_image/stb_image.h"

namespace Heart
{
    OpenGLTexture::OpenGLTexture(const std::string& path)
        : Texture(path)
    {

    }

    OpenGLTexture::~OpenGLTexture()
    {

    }
}