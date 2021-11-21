#pragma once

#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(int width, int height, int channels, void* data, u32 arrayCount, bool floatComponents);
        ~OpenGLTexture() override;

        inline u32 GetTextureId() const { return m_TextureId; }

    private:
        u32 m_TextureId;
    };
}