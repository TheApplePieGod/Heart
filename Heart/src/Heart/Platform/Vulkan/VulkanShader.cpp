#include "hepch.h"
#include "VulkanShader.h"

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "Heart/Platform/Vulkan/VulkanDevice.h"
#include "shaderc/shaderc.hpp"

namespace Heart
{
    VulkanShader::VulkanShader(const HStringView8& path, Type shaderType)
        : Shader(path, shaderType)
    {
        VulkanDevice& device = VulkanContext::GetDevice();

        HVector<u32> compiled = CompileSpirvFromFile(path, shaderType);
        Reflect(shaderType, compiled);

        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = compiled.GetCount() * sizeof(u32);
        createInfo.pCode = compiled.Data();

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