#pragma once

#include "Heart/Renderer/Texture.h"

namespace Heart
{
    class OpenGLBuffer;
    class OpenGLTexture : public Texture
    {
    public:
        OpenGLTexture(const TextureCreateInfo& createInfo, void* initialData);
        ~OpenGLTexture() override;

        void RegenerateMipMaps() override;
        void RegenerateMipMapsSync(Framebuffer* buffer) override { RegenerateMipMaps(); }

        void* GetPixelData() override;

        void* GetImGuiHandle(u32 layerIndex = 0, u32 mipLevel = 0) override;

        inline u32 GetTextureId() const { return m_TextureId; }
        inline int GetTarget() const { return m_Target; }
        inline u32 GetLayerTextureId(u32 layerIndex, u32 mipLevel) const { return m_ViewTextures[layerIndex * m_MipLevels + mipLevel]; }
        inline int GetFormat() const { return m_Format; }
        inline int GetInternalFormat() const { return m_InternalFormat; }
        inline ColorFormat GetGeneralFormat() const { return m_GeneralFormat; }

    private:
        std::vector<u32> m_ViewTextures;
        std::array<Ref<OpenGLBuffer>, 2> m_PixelBuffers;
        std::vector<void*> m_ImGuiHandles;

        ColorFormat m_GeneralFormat;
        int m_Format;
        int m_InternalFormat;
        u32 m_TextureId;
        int m_Target;
    };
}