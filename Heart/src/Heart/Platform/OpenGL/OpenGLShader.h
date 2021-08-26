#pragma once

#include "Heart/Renderer/Shader.h"

namespace Heart
{
    class OpenGLShader : public Shader
    {
    public:
        OpenGLShader(const std::string& path, Type shaderType);
        ~OpenGLShader() override;

    };
}