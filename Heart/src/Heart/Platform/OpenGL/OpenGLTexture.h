#pragma once

#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(const std::string& path);
        ~OpenGLTexture() override;
    };
}