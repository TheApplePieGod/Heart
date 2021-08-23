#pragma once

#include "Heart/Core/Layer.h"
#include "HeartEditor/EditorCamera.h"
#include "Heart/Events/KeyboardEvents.h"

// Testing
#include "Heart/Renderer/VertexBuffer.h"
#include "Heart/Renderer/IndexBuffer.h"
#include "Heart/Renderer/Framebuffer.h"
#include "Heart/Renderer/ShaderInput.h"
#include "Heart/Renderer/Buffer.h"
#include "Heart/Renderer/Texture.h"

namespace HeartEditor
{
    struct TestData
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec2 texCoord;
        };

        Heart::Ref<Heart::VertexBuffer> VertexBuffer;
        Heart::Ref<Heart::IndexBuffer> IndexBuffer;
        Heart::Ref<Heart::Framebuffer> SceneFramebuffer;
        Heart::ShaderRegistry ShaderRegistry;
        Heart::Ref<Heart::ShaderInputSet> ShaderInputSet;
        Heart::Ref<Heart::Buffer> FrameDataBuffer;
        Heart::Ref<Heart::Buffer> ObjectDataBuffer;
        Heart::TextureRegistry TextureRegistry;
    };

    class EditorLayer : public Heart::Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() override;

        void OnAttach() override;
        void OnUpdate(Heart::Timestep ts) override;
        void OnImGuiRender() override;
        void OnDetach() override;

        void OnEvent(Heart::Event& event) override;

    protected:
        bool KeyPressedEvent(Heart::KeyPressedEvent& event);

    private:
        TestData m_TestData;
        Heart::Scope<EditorCamera> m_EditorCamera;
        bool m_ViewportInput = false;
    };
}