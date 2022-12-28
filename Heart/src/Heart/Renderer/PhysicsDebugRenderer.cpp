#include "hepch.h"
#include "PhysicsDebugRenderer.h"

#include "Heart/Core/Camera.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Scene/Scene.h"
#include "btBulletDynamicsCommon.h"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/GraphicsPipeline.h"
#include "Flourish/Api/RenderContext.h"
#include "Flourish/Api/RenderPass.h"
#include "Flourish/Api/Framebuffer.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/Texture.h"
#include "Flourish/Api/CommandBuffer.h"
#include "Flourish/Api/RenderCommandEncoder.h"

namespace Heart
{
    PhysicsDebugRenderer::PhysicsDebugRenderer(u32 width, u32 height)
    {
        Flourish::RenderPassCreateInfo rpCreateInfo;
        rpCreateInfo.SampleCount = Flourish::MsaaSampleCount::None;
        rpCreateInfo.ColorAttachments.push_back({ Flourish::ColorFormat::RGBA8_UNORM });
        rpCreateInfo.Subpasses.push_back({
            {},
            { { Flourish::SubpassAttachmentType::Color, 0 } }
        });
        m_MainRenderPass = Flourish::RenderPass::Create(rpCreateInfo);

        Flourish::GraphicsPipelineCreateInfo pipelineCreateInfo;
        pipelineCreateInfo.VertexShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/DebugLine.vert", true)->GetShader();
        pipelineCreateInfo.FragmentShader = AssetManager::RetrieveAsset<ShaderAsset>("engine/DebugLine.frag", true)->GetShader();
        pipelineCreateInfo.VertexTopology = Flourish::VertexTopology::LineList;
        pipelineCreateInfo.VertexLayout = { Flourish::BufferDataType::Float3 };
        pipelineCreateInfo.VertexInput = true;
        pipelineCreateInfo.BlendStates = { { false } };
        pipelineCreateInfo.DepthTest = false;
        pipelineCreateInfo.DepthWrite = false;
        pipelineCreateInfo.CullMode = Flourish::CullMode::None;
        pipelineCreateInfo.WindingOrder = Flourish::WindingOrder::Clockwise;
        m_MainRenderPass->CreatePipeline("main", pipelineCreateInfo);
        
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Vertex;
        bufCreateInfo.Stride = sizeof(glm::vec3);
        bufCreateInfo.ElementCount = 5000;
        m_VertexBuffer = Flourish::Buffer::Create(bufCreateInfo);
        
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(CameraData);
        bufCreateInfo.ElementCount = 1;
        m_CameraDataBuffer = Flourish::Buffer::Create(bufCreateInfo);
        
        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.MaxEncoders = 1;
        m_MainCommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);
     
        Resize(width, height);
    }

    void PhysicsDebugRenderer::Draw(Scene* scene, const Camera& camera)
    {
        m_VertexCount = 0;
        setDebugMode(DBG_DrawWireframe);
        scene->GetPhysicsWorld().GetWorld()->setDebugDrawer(this);
        scene->GetPhysicsWorld().GetWorld()->debugDrawWorld();
        
        CameraData camData = {
            camera.GetProjectionMatrix(),
            camera.GetViewMatrix(),
        };
        m_CameraDataBuffer->SetElements(&camData, 1, 0);
        
        auto encoder = m_MainCommandBuffer->EncodeRenderCommands(m_MainFramebuffer.get());
        encoder->BindPipeline("main");
        encoder->BindPipelineBufferResource(0, m_CameraDataBuffer.get(), 0, 0, 1);
        encoder->FlushPipelineBindings();
        encoder->BindVertexBuffer(m_VertexBuffer.get());
        encoder->Draw(m_VertexCount, 0, 1);
        encoder->EndEncoding();
    }

    void PhysicsDebugRenderer::Resize(u32 width, u32 height)
    {
        m_RenderWidth = width;
        m_RenderHeight = height;
        
        CreateTextures();
        CreateFramebuffers();
    }

    void PhysicsDebugRenderer::drawLine(const btVector3 &from, const btVector3 &to, const btVector3 &color)
    {
        m_VertexBuffer->SetElements(&from, 1, m_VertexCount);
        m_VertexBuffer->SetElements(&to, 1, m_VertexCount + 1);
        
        m_VertexCount += 2;
    }

    void PhysicsDebugRenderer::drawContactPoint(const btVector3 &PointOnB, const btVector3 &normalOnB, btScalar distance, int lifeTime, const btVector3 &color)
    {
        
    }

    void PhysicsDebugRenderer::reportErrorWarning(const char *warningString)
    {
        
    }

    void PhysicsDebugRenderer::draw3dText(const btVector3 &location, const char *textString)
    {
        
    }

    void PhysicsDebugRenderer::CreateTextures()
    {
        Flourish::TextureCreateInfo texCreateInfo;
        texCreateInfo.Width = m_RenderWidth;
        texCreateInfo.Height = m_RenderHeight;
        texCreateInfo.Format = Flourish::ColorFormat::RGBA8_UNORM;
        texCreateInfo.Usage = Flourish::TextureUsageType::RenderTarget;
        texCreateInfo.Writability = Flourish::TextureWritability::PerFrame;
        texCreateInfo.ArrayCount = 1;
        texCreateInfo.MipCount = 1;
        m_FinalTexture = Flourish::Texture::Create(texCreateInfo);
    }

    void PhysicsDebugRenderer::CreateFramebuffers()
    {
        Flourish::FramebufferCreateInfo fbCreateInfo;
        fbCreateInfo.RenderPass = m_MainRenderPass;
        fbCreateInfo.Width = m_RenderWidth;
        fbCreateInfo.Height = m_RenderHeight;
        fbCreateInfo.ColorAttachments.push_back({ { 0.f, 0.f, 0.f, 0.f }, m_FinalTexture });
        m_MainFramebuffer = Flourish::Framebuffer::Create(fbCreateInfo);
    }
}
