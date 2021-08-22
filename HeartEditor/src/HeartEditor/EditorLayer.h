#pragma once

#include "Heart/Core/Layer.h"
#include "HeartEditor/EditorCamera.h"

// Testing
#include "Heart/Renderer/VertexBuffer.h"
#include "Heart/Renderer/IndexBuffer.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/ShaderInput.h"
#include "Heart/Renderer/Buffer.h"

namespace HeartEditor
{
    struct TestData
    {
        Heart::Ref<Heart::VertexBuffer> VertexBuffer;
        Heart::Ref<Heart::IndexBuffer> IndexBuffer;
        Heart::Ref<Heart::Framebuffer> SceneFramebuffer;
        Heart::ShaderRegistry ShaderRegistry;
        Heart::Ref<Heart::ShaderInputSet> ShaderInputSet;
        Heart::Ref<Heart::Buffer> FrameDataBuffer;
    };

    class EditorLayer : public Heart::Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override;

        void OnAttach() override;
        void OnUpdate() override;
        void OnImGuiRender() override;
        void OnDetach() override;

    private:
        TestData m_TestData;
        Heart::Scope<EditorCamera> m_EditorCamera;
    };
}