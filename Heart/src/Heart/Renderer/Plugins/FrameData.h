#pragma once

#include "glm/mat4x4.hpp"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Buffer;
    class Texture;
}

namespace Heart::RenderPlugins
{
    class FrameData : public RenderPlugin
    {
    public:
        struct BufferData
        {
            glm::mat4 Proj;
            glm::mat4 View;
            glm::mat4 InvProj;
            glm::mat4 InvView;
            glm::mat4 InvViewProj;
            glm::vec4 CameraPos;
            glm::vec2 ClipPlanes;
            glm::vec2 ScreenSize;
            u32 ReverseDepth;
            float Padding;
        };

    public:
        FrameData(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        { Initialize(); }

        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        Ref<Flourish::Buffer> m_Buffer;
    };
}
