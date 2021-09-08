#include "htpch.h"
#include "Framebuffer.h"

#include "Heart/Renderer/Renderer.h"
#include "Heart/Platform/Vulkan/VulkanFramebuffer.h"
#include "Heart/Platform/OpenGL/OpenGLFramebuffer.h"
#include "Heart/Core/Window.h"

namespace Heart
{
    Framebuffer::Framebuffer(const FramebufferCreateInfo& createInfo)
        : m_Info(createInfo)
    {
        SubscribeToEmitter(&Window::GetMainWindow());
    }

    Framebuffer::~Framebuffer()
    {
        UnsubscribeFromEmitter(&Window::GetMainWindow());
    }

    Ref<Framebuffer> Framebuffer::Create(const FramebufferCreateInfo& createInfo)
    {
        HE_ENGINE_LOG_TRACE("Creating framebuffer");
        switch (Renderer::GetApiType())
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create Framebuffer: selected ApiType is not supported"); return nullptr; }
            case RenderApi::Type::Vulkan:
            { return CreateRef<VulkanFramebuffer>(createInfo); }
            case RenderApi::Type::OpenGL:
            { return CreateRef<OpenGLFramebuffer>(createInfo); }
        }
    }

    Ref<GraphicsPipeline> Framebuffer::RegisterGraphicsPipeline(const std::string& name, const GraphicsPipelineCreateInfo& createInfo)
    {
        if (m_GraphicsPipelines.find(name) != m_GraphicsPipelines.end())
        {
            HE_ENGINE_LOG_ERROR("Cannot register pipeline, name already exists: {0}", name);
            HE_ENGINE_ASSERT(false);
        }

        HE_ENGINE_LOG_TRACE("Registering graphics pipeline '{0}' to framebuffer", name);

        Ref<GraphicsPipeline> newPipeline = InternalInitializeGraphicsPipeline(createInfo);
        m_GraphicsPipelines[name] = newPipeline;
        return newPipeline;
    }

    Ref<GraphicsPipeline> Framebuffer::LoadPipeline(const std::string& name)
    {
        HE_PROFILE_FUNCTION()

        if (m_GraphicsPipelines.find(name) == m_GraphicsPipelines.end())
        {
            HE_ENGINE_LOG_ERROR("Pipeline not registered: {0}", name);
            HE_ENGINE_ASSERT(false);
        }
        return m_GraphicsPipelines[name];
    }

    void Framebuffer::OnEvent(Event& event)
    {
        event.Map<WindowResizeEvent>(HE_BIND_EVENT_FN(Framebuffer::OnWindowResize));
    }

    bool Framebuffer::OnWindowResize(WindowResizeEvent& event)
    {
        //HE_ENGINE_LOG_INFO("FB window resized");

        if (event.GetWidth() == 0 || event.GetHeight() == 0)
            return false;

        u32 newWidth = m_Info.Width == 0 ? event.GetWidth() : m_Info.Width;
        u32 newHeight = m_Info.Height == 0 ? event.GetHeight() : m_Info.Height;

        Invalidate(newWidth, newHeight);        

        return false;
    }

    void Framebuffer::Invalidate(u32 newWidth, u32 newHeight)
    {
        m_Valid = false;
        m_ActualWidth = newWidth;
        m_ActualHeight = newHeight;
    }
}