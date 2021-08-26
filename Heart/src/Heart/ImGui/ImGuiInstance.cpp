#include "htpch.h"
#include "ImGuiInstance.h"

#include "Heart/Core/App.h"
#include "imgui/imgui.h"
#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanContext.h"
#include "imgui/backends/imgui_impl_glfw.h"

namespace Heart
{
    void ImGuiInstance::Initialize()
    {
		HE_ENGINE_LOG_TRACE("Initializing ImGui instance");
        auto& window = App::Get().GetWindow();

        IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
		io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows (disabling this for now because it's causing a ton of issues with vulkan)
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

        //SetThemeColors();

        // do the Inits here because they don't need to be recalled when recreating the instance
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot initialize ImGui: selected ApiType is not supported"); } break;
            case RenderApi::Type::Vulkan:
            { ImGui_ImplGlfw_InitForVulkan(window.GetWindowHandle(), true); } break;
			case RenderApi::Type::OpenGL:
            { ImGui_ImplGlfw_InitForOpenGL(window.GetWindowHandle(), true); } break;
        }

        Recreate();
    }

    void ImGuiInstance::Shutdown()
    {
		HE_ENGINE_LOG_TRACE("Shutting down ImGui instance");
        Cleanup();
        
        ImGui_ImplGlfw_Shutdown();
	    ImGui::DestroyContext();
    }

    void ImGuiInstance::Recreate()
    {
        if (m_Initialized)
            Cleanup();

        App::Get().GetWindow().GetContext().InitializeImGui();

        m_Initialized = true;
    }

    void ImGuiInstance::Cleanup()
    {
        if (!m_Initialized) return;

        App::Get().GetWindow().GetContext().ShutdownImGui();

        m_Initialized = false;
    }

    void ImGuiInstance::BeginFrame()
    {
        App::Get().GetWindow().GetContext().ImGuiBeginFrame();

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
    }

    void ImGuiInstance::EndFrame()
    {  
        ImGuiIO& io = ImGui::GetIO();
		Window& mainWindow = App::Get().GetWindow();
		io.DisplaySize = ImVec2((f32)mainWindow.GetWidth(), (f32)mainWindow.GetHeight());
        
		ImGui::Render();
        mainWindow.GetContext().ImGuiEndFrame();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			GLFWwindow* backup_current_context = glfwGetCurrentContext();
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
			glfwMakeContextCurrent(backup_current_context);
		}
    }

    void ImGuiInstance::SetThemeColors()
    {
        auto& colors = ImGui::GetStyle().Colors;
		colors[ImGuiCol_WindowBg] = ImVec4{ 0.1f, 0.105f, 0.11f, 1.0f };

		// Headers
		colors[ImGuiCol_Header] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_HeaderHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_HeaderActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		
		// Buttons
		colors[ImGuiCol_Button] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_ButtonHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_ButtonActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Frame BG
		colors[ImGuiCol_FrameBg] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };
		colors[ImGuiCol_FrameBgHovered] = ImVec4{ 0.3f, 0.305f, 0.31f, 1.0f };
		colors[ImGuiCol_FrameBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };

		// Tabs
		colors[ImGuiCol_Tab] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabHovered] = ImVec4{ 0.38f, 0.3805f, 0.381f, 1.0f };
		colors[ImGuiCol_TabActive] = ImVec4{ 0.28f, 0.2805f, 0.281f, 1.0f };
		colors[ImGuiCol_TabUnfocused] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TabUnfocusedActive] = ImVec4{ 0.2f, 0.205f, 0.21f, 1.0f };

		// Title
		colors[ImGuiCol_TitleBg] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgActive] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
		colors[ImGuiCol_TitleBgCollapsed] = ImVec4{ 0.15f, 0.1505f, 0.151f, 1.0f };
    }
}