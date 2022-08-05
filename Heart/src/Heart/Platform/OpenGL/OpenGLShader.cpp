#include "hepch.h"
#include "OpenGLShader.h"

#include "glad/glad.h"
#include "shaderc/shaderc.hpp"
#include "Heart/Platform/OpenGL/OpenGLCommon.h"

namespace Heart
{
    OpenGLShader::OpenGLShader(const HString& path, Type shaderType)
        : Shader(path, shaderType)
    {
        m_ShaderId = glCreateShader(OpenGLCommon::ShaderTypeToOpenGL(shaderType));

        std::vector<u32> compiled = CompileSpirvFromFile(path, shaderType);
        Reflect(shaderType, compiled);

        glShaderBinary(1, &m_ShaderId, GL_SHADER_BINARY_FORMAT_SPIR_V, compiled.data(), static_cast<int>(compiled.size() * sizeof(u32)));
        glSpecializeShader(m_ShaderId, "main", 0, nullptr, nullptr);
    }

    OpenGLShader::~OpenGLShader()
    {
        glDeleteShader(m_ShaderId);
    }
}