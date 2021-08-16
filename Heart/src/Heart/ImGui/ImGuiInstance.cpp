#include "htpch.h"
#include "ImGuiInstance.h"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_glfw.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    Scope<ImGuiInstance> ImGuiInstance::Create()
    {
        return CreateScope<ImGuiInstance>();
    }

    ImGuiInstance::ImGuiInstance()
    {
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        //ImGui_ImplGlfw_InitForVulkan(window, true);

        Recreate();
    }

    ImGuiInstance::~ImGuiInstance()
    {
        Cleanup();
    }

    void ImGuiInstance::Recreate()
    {
        if (m_Initialized)
            Cleanup();

        // ImGui_ImplVulkan_InitInfo initInfo = ImGuiInitInfo();
        // ImGui_ImplVulkan_Init(&initInfo, renderPass);
        // VkCommandBuffer commandBuffer = BeginSingleTimeCommands();
        // ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);
        // EndSingleTimeCommands(commandBuffer);
        // ImGui_ImplVulkan_DestroyFontUploadObjects();

        m_Initialized = true;
    }

    void ImGuiInstance::Cleanup()
    {
        m_Initialized = false;
    }
}