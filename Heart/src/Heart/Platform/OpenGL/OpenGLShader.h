#pragma once

#include "Heart/Renderer/Shader.h"

namespace Heart
{
    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const HString& path, Type shaderType);
        ~OpenGLShader() override;

        inline u32 GetShaderId() const { return m_ShaderId; }

    private:
        u32 m_ShaderId;
    };
}