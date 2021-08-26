#pragma once

#include "Heart/Renderer/ShaderInput.h"

namespace Heart
{
    class OpenGLShaderInputSet : public ShaderInputSet
    {
    public:
        OpenGLShaderInputSet(std::initializer_list<ShaderInputElement> elements);
        ~OpenGLShaderInputSet() override;

        ShaderInputBindPoint CreateBindPoint(const std::vector<ShaderInputBindElement>& bindElements) override;
    };
}