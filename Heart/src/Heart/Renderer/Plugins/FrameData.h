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
            glm::mat4 PrevViewProj;
            glm::mat4 InvProj;
            glm::mat4 InvView;
            glm::mat4 InvViewProj;
            glm::vec4 CameraPos;
            glm::vec2 ClipPlanes;
            glm::vec2 ScreenSize;
            u32 ReverseDepth;
            u32 FrameCount;
        };

    public:
        FrameData(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        {}

        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        Ref<Flourish::Buffer> m_Buffer;
        glm::mat4 m_PrevViewProj;
    };
}
