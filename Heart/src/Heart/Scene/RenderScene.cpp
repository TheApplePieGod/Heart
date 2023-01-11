#include "hepch.h"
#include "RenderScene.h"

#include "Heart/Scene/Entity.h"
#include "Heart/Core/Timing.h"

namespace Heart
{
    void RenderScene::CopyFromScene(Scene* scene)
    {
        auto timer = AggregateTimer("RenderScene::CopyFromScene");

        m_EntityData.Clear();
        m_MeshComponents.Clear();
        m_TextComponents.Clear();
        m_LightComponents.Clear();

        scene->GetRegistry().each([&](auto handle)
        {
            Entity src = { scene, handle };
            
            u32 entityIndex = m_EntityData.Count();
            bool shouldAdd = false;

            if (src.HasComponent<MeshComponent>())
            {
                m_MeshComponents.AddInPlace(src.GetComponent<MeshComponent>(), entityIndex);
                shouldAdd = true;
            }

            if (src.HasComponent<TextComponent>())
            {
                m_TextComponents.AddInPlace(src.GetComponent<TextComponent>(), entityIndex);
                shouldAdd = true;
            }

            if (src.HasComponent<LightComponent>())
            {
                m_LightComponents.AddInPlace(src.GetComponent<LightComponent>(), entityIndex);
                shouldAdd = true;
            }
            
            if (!shouldAdd)
                return;
            
            const auto& cachedData = scene->GetEntityCachedData(src);
            m_EntityData.AddInPlace(
                (u32)handle,
                cachedData.Transform,
                cachedData.Position,
                cachedData.Rotation,
                cachedData.Scale,
                cachedData.ForwardVec
            );
        });
    }
}