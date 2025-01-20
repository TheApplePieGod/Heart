#include "hepch.h"
#include "ImGuiInstance.h"

#include "Heart/Core/App.h"
#include "Heart/Core/Window.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Util/FilesystemUtils.h"
#include "imgui/imgui.h"

#ifdef HE_PLATFORM_ANDROID
#include "imgui/backends/imgui_impl_android.h"
#else
#include "GLFW/glfw3.h"
#include "imgui/backends/imgui_impl_glfw.h"
#endif

#include "Flourish/Backends/Vulkan/Context.h"
#include "Flourish/Backends/Vulkan/RenderPass.h"
#include "Flourish/Backends/Vulkan/RenderCommandEncoder.h"
#include "imgui/backends/imgui_impl_vulkan.h"

namespace Heart
{
    ImGuiInstance::ImGuiInstance(Ref<Window>& window)
		: m_Window(window)
    {
		HE_ENGINE_LOG_TRACE("Initializing ImGui instance");

        IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.IniFilename = nullptr;
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;       // Enable Keyboard Controls
		//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
        //io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoTaskBarIcons;
		//io.ConfigFlags |= ImGuiConfigFlags_ViewportsNoMerge;

        #ifndef HE_DIST
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;           // Enable Docking
            io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;         // Enable Multi-Viewport / Platform Windows
        #endif

		// Setup Dear ImGui style
		ImGui::StyleColorsDark();
		//ImGui::StyleColorsClassic();

		// When viewports are enabled we tweak WindowRounding/WindowBg so platform windows can look identical to regular ones
		ImGuiStyle& style = ImGui::GetStyle();
		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			style.WindowRounding = 0.0f;
			style.Colors[ImGuiCol_WindowBg].w = 1.0f;
		}

        SetThemeColors();

        UploadFonts();

        Recreate();
    }

    ImGuiInstance::~ImGuiInstance()
    {
		HE_ENGINE_LOG_TRACE("Shutting down ImGui instance");

        Cleanup(false);
        
	    ImGui::DestroyContext();
    }

    void ImGuiInstance::UpdateWindow(Ref<Window>& window)
    {
        m_Window = window;
        Recreate();
    }

    void ImGuiInstance::Recreate()
    {
        if (m_Initialized)
            Cleanup(true);

		HE_ENGINE_LOG_TRACE("ImGui: Recreating");

        switch (Flourish::Context::BackendType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot initialize ImGui: selected ApiType is not supported"); } break;
            case Flourish::BackendType::Vulkan:
            {
                #ifdef HE_PLATFORM_ANDROID
                    ImGui_ImplAndroid_Init((ANativeWindow*)m_Window->GetWindowHandle());
                #else
                    ImGui_ImplGlfw_InitForVulkan((GLFWwindow*)m_Window->GetWindowHandle(), true);
                #endif

    			ImGui_ImplVulkan_LoadFunctions([](const char* function_name, void*)
				{
					return vkGetInstanceProcAddr(Flourish::Vulkan::Context::Instance(), function_name);
				});

		        HE_ENGINE_LOG_TRACE("ImGui: Vulkan functions loaded");

                if (!m_DescriptorPool)
                {
                    // Create the descriptor pool for imgui
                    std::array<VkDescriptorPoolSize, 1> poolSizes{};
                    poolSizes[0].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                    poolSizes[0].descriptorCount = 10000;
                    VkDescriptorPoolCreateInfo poolInfo{};
                    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
                    poolInfo.poolSizeCount = static_cast<u32>(poolSizes.size());
                    poolInfo.pPoolSizes = poolSizes.data();
                    poolInfo.maxSets = 10000;
                    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
                    vkCreateDescriptorPool(
                        Flourish::Vulkan::Context::Devices().Device(),
                        &poolInfo,
                        nullptr,
                        (VkDescriptorPool*)&m_DescriptorPool
                    );

                    HE_ENGINE_LOG_TRACE("ImGui: Descriptor pool created");
                }

				// Initialize
				ImGui_ImplVulkan_InitInfo info = {};
				info.Instance = Flourish::Vulkan::Context::Instance();
				info.PhysicalDevice = Flourish::Vulkan::Context::Devices().PhysicalDevice();
				info.Device = Flourish::Vulkan::Context::Devices().Device();
				info.QueueFamily = Flourish::Vulkan::Context::Queues().QueueIndex(Flourish::GPUWorkloadType::Graphics);
				info.Queue = Flourish::Vulkan::Context::Queues().Queue(Flourish::GPUWorkloadType::Graphics);
				info.PipelineCache = VK_NULL_HANDLE;
				info.DescriptorPool = (VkDescriptorPool)m_DescriptorPool;
				info.Allocator = NULL;
				info.MinImageCount = 2;
                info.DescriptorSetLayout = (VkDescriptorSetLayout)m_DescriptorSetLayout;
				info.ImageCount = Flourish::Context::FrameBufferCount();
                info.RenderPass = ((Flourish::Vulkan::RenderPass*)App::Get().GetWindow().GetRenderContext()->GetRenderPass())->GetRenderPass();
				#ifdef HE_DEBUG
					info.CheckVkResultFn = [](VkResult err)
					{
						HE_ENGINE_ASSERT(err == VK_SUCCESS, "Vulkan function failed");
					};
				#endif
				info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;

				ImGui_ImplVulkan_Init(&info);

                m_DescriptorSetLayout = ImGui_ImplVulkan_GetDescriptorSetLayout();

		        HE_ENGINE_LOG_TRACE("ImGui: Vulkan initialized");
			} break;
        }

        m_Initialized = true;
    }

	void ImGuiInstance::OverrideImGuiConfig(const HStringView8& newBasePath)
	{
		ImGuiIO& io = ImGui::GetIO();

		m_ImGuiConfigPath = std::filesystem::path(newBasePath.Data()).append("imgui.ini").u8string();
		io.IniFilename = m_ImGuiConfigPath.Data();
	}

	void ImGuiInstance::ReloadImGuiConfig()
	{
		ImGuiIO& io = ImGui::GetIO();

		ImGui::LoadIniSettingsFromDisk(io.IniFilename);
	}

    void ImGuiInstance::Cleanup(bool willRecreate)
    {
        if (!m_Initialized) return;

        if (!willRecreate)
            HE_LOG_DEBUG("Fully cleaning up ImGuiInstance");

        // Special case when we want to cleanup/recreate in the middle of an imgui frame.
        // End frame prematurely so that old draw data does not get rendered with new device
        // objects
        EndFrameInternal(false);

        switch (Flourish::Context::BackendType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot cleanup ImGui: selected ApiType is not supported"); } break;
            case Flourish::BackendType::Vulkan:
            {
                Flourish::Vulkan::Context::Sync();

				ImGui_ImplVulkan_Shutdown(willRecreate);
                if (!willRecreate)
                {
                    vkDestroyDescriptorPool(
                        Flourish::Vulkan::Context::Devices().Device(),
                        (VkDescriptorPool)m_DescriptorPool,
                        nullptr
                    );
                    m_DescriptorPool = nullptr;
                }
			} break;
		}

        #ifdef HE_PLATFORM_ANDROID
            ImGui_ImplAndroid_Shutdown();
        #else
            ImGui_ImplGlfw_Shutdown();
        #endif

        m_Initialized = false;
    }

    void ImGuiInstance::BeginFrame()
    {
		HE_PROFILE_FUNCTION();

        if (m_InFrame) return;

        switch (Flourish::Context::BackendType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot begin new ImGui frame: selected ApiType is not supported"); } break;
            case Flourish::BackendType::Vulkan:
            { ImGui_ImplVulkan_NewFrame(); } break;
		}

        #ifdef HE_PLATFORM_ANDROID
            ImGui_ImplAndroid_NewFrame();
        #else
            ImGui_ImplGlfw_NewFrame();
        #endif
        ImGui::NewFrame();

        m_InFrame = true;
    }

    void ImGuiInstance::EndFrame()
    {  
		HE_PROFILE_FUNCTION();

        EndFrameInternal(true);
    }

    void ImGuiInstance::EndFrameInternal(bool render)
    {
        if (!m_InFrame) return;
		
        ImGuiIO& io = ImGui::GetIO();
		io.DisplaySize = ImVec2((f32)m_Window->GetWidth(), (f32)m_Window->GetHeight());
        
        if (render)
        {
            ImGui::Render();
            switch (Flourish::Context::BackendType())
            {
                default:
                { HE_ENGINE_ASSERT(false, "Cannot end ImGui frame: selected ApiType is not supported"); } break;
                case Flourish::BackendType::Vulkan:
                {
                    auto encoder = (Flourish::Vulkan::RenderCommandEncoder*)App::Get().GetWindow().GetRenderContext()->EncodeRenderCommands();
                    ImGui_ImplVulkan_RenderDrawData(ImGui::GetDrawData(), encoder->GetCommandBuffer());
                    encoder->MarkManuallyRecorded();
                    encoder->EndEncoding();
                } break;
            }
        }
        else
            ImGui::EndFrame();

		if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
		{
			ImGui::UpdatePlatformWindows();
			ImGui::RenderPlatformWindowsDefault();
		}

        m_InFrame = false;
    }

    void ImGuiInstance::UploadFonts()
    {
        ImGuiIO& io = ImGui::GetIO();

        float dpiScale = m_Window->GetDPIScale();

        // Small font
        {
            u32 fontDataSize;
            unsigned char* fontData = FilesystemUtils::ReadFile("resources/engine/Inter-Regular.otf", fontDataSize);
            m_SmallFont = io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 14.f * dpiScale);
            io.FontDefault = m_SmallFont;
        }

        // Medium font
        {
            u32 fontDataSize;
            unsigned char* fontData = FilesystemUtils::ReadFile("resources/engine/Inter-Regular.otf", fontDataSize);
            m_MediumFont = io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 18.f * dpiScale);
        }

        // Large font
        {
            u32 fontDataSize;
            unsigned char* fontData = FilesystemUtils::ReadFile("resources/engine/Inter-Regular.otf", fontDataSize);
            m_LargeFont = io.Fonts->AddFontFromMemoryTTF(fontData, fontDataSize, 24.f * dpiScale);
        }

        io.FontGlobalScale = 1.f / dpiScale;
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
