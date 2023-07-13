#include "hepch.h"
#include "FrameData.h"

#include "glm/gtc/matrix_transform.hpp"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart::RenderPlugins
{
    void FrameData::InitializeInternal()
    {
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Uniform;
        bufCreateInfo.Stride = sizeof(BufferData);
        bufCreateInfo.ElementCount = 1;
        m_Buffer = Flourish::Buffer::Create(bufCreateInfo);
    }

    void FrameData::RenderInternal(const SceneRenderData& data)
    {
        BufferData bufData = {
            data.Camera->GetProjectionMatrix(),
            data.Camera->GetViewMatrix(),
            m_PrevViewProj,
            glm::inverse(data.Camera->GetProjectionMatrix()),
            glm::inverse(data.Camera->GetViewMatrix()),
            glm::inverse(data.Camera->GetProjectionMatrix() * data.Camera->GetViewMatrix()),
            glm::vec4(data.CameraPos, 1.f),
            glm::vec2(data.Camera->GetNearClip(), data.Camera->GetFarClip()),
            { m_Renderer->GetRenderWidth(), m_Renderer->GetRenderHeight() },
            Flourish::Context::ReversedZBuffer(),
            (u32)Flourish::Context::FrameCount()
        };
        m_Buffer->SetElements(&bufData, 1, 0);

        m_PrevViewProj = bufData.Proj * bufData.View;
    }
}
