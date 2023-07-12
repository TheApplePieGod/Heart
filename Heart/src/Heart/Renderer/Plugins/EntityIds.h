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
    class EntityIds : public RenderPlugin
    {
    public:
        EntityIds(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        {}

        inline Ref<Flourish::Texture>& GetTexture() { return m_Texture; }
        inline const Flourish::Buffer* GetBuffer() const { return m_Buffer.get(); }

    protected:
        void InitializeInternal() override;
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override;

    private:
        Ref<Flourish::Texture> m_Texture;
        Ref<Flourish::Buffer> m_Buffer;
    };
}
