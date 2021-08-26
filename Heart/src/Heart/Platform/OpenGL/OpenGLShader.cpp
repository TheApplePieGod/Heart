#include "htpch.h"
#include "OpenGLShader.h"

#include "shaderc/shaderc.hpp"

namespace Heart
{
    OpenGLShader::OpenGLShader(const std::string& path, Type shaderType)
        : Shader(path, shaderType)
    {

    }

    OpenGLShader::~OpenGLShader()
    {

    }
}