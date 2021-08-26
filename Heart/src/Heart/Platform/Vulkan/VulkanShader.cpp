#include "htpch.h"
#include "VulkanShader.h"

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "shaderc/shaderc.hpp"

namespace Heart
{
    VulkanShader::VulkanShader(const std::string& path, Type shaderType)
        : Shader(path, shaderType)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        std::vector<u32> compiled = CompileSpirvFromFile(path, shaderType);
        Reflect(shaderType, compiled);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = compiled.size() * sizeof(u32);
        createInfo.pCode = compiled.data();

        HE_VULKAN_CHECK_RESULT(vkCreateShaderModule(device.Device(), &createInfo, nullptr, &m_ShaderModule));

        m_Loaded = true;
    }

    VulkanShader::~VulkanShader()
    {
        if (!m_Loaded) return;

        VulkanDevice& device = VulkanContext::GetDevice();

        vkDestroyShaderModule(device.Device(), m_ShaderModule, nullptr);
    }
}