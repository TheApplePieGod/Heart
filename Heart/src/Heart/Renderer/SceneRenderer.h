#pragma once

#include "Heart/Renderer/FrameBuffer.h"
#include "Heart/Renderer/GraphicsContext.h"

namespace Heart
{
    class SceneRenderer
    {
    public:
        static void Initialize();
        static void Shutdown();

        static void Bind();
        static void Render(GraphicsContext& context);
        inline static FrameBuffer& GetBuf() { return *buf; }

    private:
        static Ref<FrameBuffer> buf;
    };
}