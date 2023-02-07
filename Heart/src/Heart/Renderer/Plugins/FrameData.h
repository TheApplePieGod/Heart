#pragma once

#include "glm/mat4x4.hpp"
#include "Heart/Renderer/RenderPlugin.h"

namespace Flourish
{
    class Buffer;
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
            glm::mat4 InvViewProj;
            glm::vec4 CameraPos;
            glm::vec2 ScreenSize;
            u32 ReverseDepth;
            u32 CullEnable;
            u32 BloomEnable;
            u32 SSAOEnable;
            u32 PhysicsDebugEnable;
            float Padding;
        };

    public:
        FrameData(HStringView8 name)
            : RenderPlugin(name)
        { Initialize(); }

        void Resize(u32 width, u32 height) override {}
        
        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer) override;

    private:
        void Initialize();

    private:
        Ref<Flourish::Buffer> m_Buffer;
    };
}