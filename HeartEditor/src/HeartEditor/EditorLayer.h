#pragma once

#include "Heart/Core/Layer.h"

// Testing
#include "Heart/Renderer/VertexBuffer.h"
#include "Heart/Renderer/IndexBuffer.h"
#include "Heart/Renderer/FrameBuffer.h"

namespace HeartEditor
{
    struct TestData
    {
        Heart::Ref<Heart::VertexBuffer> VertexBuffer;
        Heart::Ref<Heart::IndexBuffer> IndexBuffer;
        Heart::Ref<Heart::FrameBuffer> SceneFrameBuffer;
        Heart::ShaderRegistry ShaderRegistry;
    };

    class EditorLayer : public Heart::Layer
    {
    public:
        void OnAttach() override;
        void OnUpdate() override;
        void OnImGuiRender() override;
        void OnDetach() override;

    private:
        TestData m_TestData;
    };
}