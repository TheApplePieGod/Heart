#pragma once

#include "Heart/Renderer/Shader.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanShader : public Shader
    {
    public:
        VulkanShader(const HString& path, Type shaderType);
        ~VulkanShader() override;

        inline VkShaderModule GetShaderModule() const { return m_ShaderModule; }

    private:
        VkShaderModule m_ShaderModule;

    };
}