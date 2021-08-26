#include "htpch.h"
#include "OpenGLShaderInput.h"

namespace Heart
{
    OpenGLShaderInputSet::OpenGLShaderInputSet(std::initializer_list<ShaderInputElement> elements)
        : ShaderInputSet(elements)
    {

    }

    OpenGLShaderInputSet::~OpenGLShaderInputSet()
    {

    }

    ShaderInputBindPoint OpenGLShaderInputSet::CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements)
    {
        return ShaderInputBindPoint();
    }
}