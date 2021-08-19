#include "htpch.h"
#include "VulkanRenderApi.h"

#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "GLFW/glfw3.h"

namespace Heart
{
    VulkanRenderApi::VulkanRenderApi()
    {
        HE_ENGINE_LOG_TRACE("Initializing VulkanRenderApi");

        
    }

    VulkanRenderApi::~VulkanRenderApi()
    {

    }

    void VulkanRenderApi::SetViewport(GraphicsContext& _context, u32 x, u32 y, u32 width, u32 height)
    {
        VulkanContext& context = static_cast<VulkanContext&>(_context);
        context.GetSwapChain().InvalidateSwapChain(width, height);
    }
}