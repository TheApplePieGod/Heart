#include "htpch.h"
#include "ImGuiInstance.h"

#include "Heart/Core/Engine.h"
#include "imgui/imgui.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "imgui/backends/imgui_impl_glfw.h"

namespace Heart
{
    void ImGuiInstance::Initialize()
    {
        auto& window = Engine::Get().GetWindow();

        IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

		//io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Bold.ttf", 18.0f);
		//io.FontDefault = io.Fonts->AddFontFromFileTTF("assets/fonts/opensans/OpenSans-Regular.ttf", 18.0f);

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones.
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

        // do the Inits here because they don't need to be recalled when recreating the instance
        switch (Renderer::GetApiType())
        {
            default:
            { HT_ENGINE_ASSERT(false, "Cannot cleanup ImGui: selected ApiType is not supported"); } break;
            case RenderApi::Type::Vulkan:
            { ImGui_ImplGlfw_InitForVulkan(window.GetWindowHandle(), true); } break;
        }

        Recreate();
    }

    void ImGuiInstance::Shutdown()
    {
        Cleanup();
        
        ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
    }

    void ImGuiInstance::Recreate()
    {
        if (m_Initialized)
            Cleanup();

        Engine::Get().GetWindow().GetContext().InitializeImGui();

        m_Initialized = true;
    }

    void ImGuiInstance::Cleanup()
    {
        if (!m_Initialized) return;

        Engine::Get().GetWindow().GetContext().ShutdownImGui();

        m_Initialized = false;
    }

    void ImGuiInstance::BeginFrame()
    {
        Engine::Get().GetWindow().GetContext().ImGuiBeginFrame();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiInstance::EndFrame()
    {  
        ImGuiIO& io = ImGui::GetIO();
		Window& mainWindow = Engine::Get().GetWindow();
		io.DisplaySize = ImVec2((f32)mainWindow.GetWidth(), (f32)mainWindow.GetHeight());

		ImGui::Render();
        mainWindow.GetContext().ImGuiBeginFrame();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
    }
}