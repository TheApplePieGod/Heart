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

        void SetViewport(GraphicsContext& context, u32 x, u32 y, u32 width, u32 height) override;

    private:
    };
}