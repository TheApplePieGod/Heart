#pragma once

#include "Heart/Renderer/SceneRenderer.h"

namespace Heart
{
    class DesktopSceneRenderer : public SceneRenderer
    {
    public:
        DesktopSceneRenderer() = default;

        inline Ref<Flourish::Texture>& GetDepthTexture() { return m_DepthTexture; }

    protected:
        void RegisterPlugins() override;
        void CreateResources() override;

    private:
        Ref<Flourish::Texture> m_RayReflectionsTexture;
        Ref<Flourish::Texture> m_DepthTexture;
    };
}
