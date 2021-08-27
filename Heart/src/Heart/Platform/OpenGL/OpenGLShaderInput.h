#pragma once

#include "Heart/Renderer/ShaderInput.h"
#include "Heart/Platform/OpenGL/OpenGLBuffer.h"
#include "Heart/Platform/OpenGL/OpenGLTexture.h"

namespace Heart
{
    class OpenGLShaderInputSet : public ShaderInputSet
    {
    public:
        struct BindData
        {
            std::array<OpenGLBuffer*, 100> Buffers;
            std::array<OpenGLTexture*, 100> Textures;
        };

    public:
        OpenGLShaderInputSet(std::initializer_list<ShaderInputElement> elements);
        ~OpenGLShaderInputSet() override;

        ShaderInputBindPoint CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements) override;

    private:
        u64 m_LastResetFrame = 0;
        std::vector<BindData> m_BindDataPool;
    };
}