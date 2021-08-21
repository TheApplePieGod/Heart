#pragma once

#include "Heart/Core/Layer.h"

// Testing
#include "Heart/Renderer/VertexBuffer.h"
#include "Heart/Renderer/IndexBuffer.h"
#include "Heart/Renderer/Framebuffer.h"

namespace HeartEditor
{
    struct TestData
    {
        Heart::Ref<Heart::VertexBuffer> VertexBuffer;
        Heart::Ref<Heart::IndexBuffer> IndexBuffer;
        Heart::Ref<Heart::Framebuffer> SceneFramebuffer;
        Heart::ShaderRegistry ShaderRegistry;
    };

    class EditorLayer : public Heart::Layer
    {
    public:
        ~EditorLayer() override;

        void OnAttach() override;
        void OnUpdate() override;
        void OnImGuiRender() override;
        void OnDetach() override;

    private:
        TestData m_TestData;
    };
}