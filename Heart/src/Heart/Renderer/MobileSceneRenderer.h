#pragma once

#include "Heart/Renderer/SceneRenderer.h"

namespace Heart
{
    class MobileSceneRenderer : public SceneRenderer
    {
    public:
        MobileSceneRenderer(bool debug)
            : SceneRenderer(debug)
        {}

        inline Ref<Flourish::Texture>& GetDepthTexture() { return m_DepthTexture; }

    protected:
        void RegisterPlugins() override;
        void CreateResources() override;

    private:
        Ref<Flourish::Texture> m_DepthTexture;
    };
}
