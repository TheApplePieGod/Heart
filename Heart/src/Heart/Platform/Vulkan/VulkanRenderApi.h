#pragma once

#include "Heart/Renderer/RenderApi.h"
#include "Heart/Platform/Vulkan/VulkanCommon.h"

namespace Heart
{
    class VulkanRenderApi : public RenderApi
    {
    public:
        VulkanRenderApi();
        ~VulkanRenderApi() override;

    private:
    };
}