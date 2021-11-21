#pragma once

#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(const TextureCreateInfo& createInfo, void* initialData);
        ~OpenGLTexture() override;

        inline u32 GetTextureId() const { return m_TextureId; }

    private:
        u32 m_TextureId;
    };
}