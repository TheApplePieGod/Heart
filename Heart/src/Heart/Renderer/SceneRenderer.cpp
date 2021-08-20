#include "htpch.h"
#include "SceneRenderer.h"

namespace Heart
{
    Ref<FrameBuffer> SceneRenderer::buf;

    void SceneRenderer::Initialize()
    {
        FrameBufferCreateInfo createInfo;
        createInfo.Width = 1920;
        createInfo.Height = 1080;
        createInfo.SampleCount = MsaaSampleCount::Max;
        createInfo.Attachments = {
            { false, false, ColorFormat::RGBA8, 0, 0 },
        };

        buf = FrameBuffer::Create(createInfo);
    }

    void SceneRenderer::Shutdown()
    {
        buf.reset();
    }

    void SceneRenderer::Bind()
    {
        buf->Bind();
    }

    void SceneRenderer::Render(GraphicsContext& context)
    {
        buf->Submit(context);
    }
}