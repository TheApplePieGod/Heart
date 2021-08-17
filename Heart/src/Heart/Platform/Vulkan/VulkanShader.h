#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const std::string& path, Type shaderType);
        ~VulkanShader() override;

    private:
         

    private:
        VkShaderModule m_ShaderModule;

    };
}