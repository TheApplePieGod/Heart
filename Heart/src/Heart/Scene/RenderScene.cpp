#include "hepch.h"
#include "RenderScene.h"

#include "Heart/Scene/Entity.h"
#include "Heart/Core/Timing.h"
#include "Heart/Task/JobManager.h"

namespace Heart
{
    void RenderScene::Cleanup()
    {
        m_Registry.clear();
    }

    void RenderScene::CopyFromScene(Scene* scene)
    {
        auto timer = AggregateTimer("RenderScene::CopyFromScene");

        Cleanup();

        auto& srcEntity = scene->GetRegistry().storage<entt::entity>();
        auto srcMesh = scene->GetRegistry().view<MeshComponent>();
        auto srcLight = scene->GetRegistry().view<LightComponent>();

        m_Registry.storage<entt::entity>().push(srcEntity.data(), srcEntity.data() + srcEntity.size());
        m_Registry.insert<MeshComponent>(srcMesh.begin(), srcMesh.end(), srcMesh.storage()->begin());
        m_Registry.insert<LightComponent>(srcLight.begin(), srcLight.end(), srcLight.storage()->begin());

        m_CachedTransforms = scene->GetCachedTransforms();

        //auto view = src.view<MeshComponent>();

        /*
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

        // Spawn jobs to recompute text data
        if (m_TextComponents.Count() > 0)
        {
            auto job = JobManager::Schedule(
                m_TextComponents.Count(),
                [this, scene](size_t index)
                {
                    auto& textComp = m_TextComponents[index];
                    textComp.Data.RecomputeRenderData();
                    
                    // Update original text component with new data so that we can cache the computed result
                    u32 entityHandle = m_EntityData[textComp.EntityIndex].Id;
                    auto& ogTextComp = scene->GetRegistry().get<TextComponent>((entt::entity)entityHandle);
                    ogTextComp.ComputedMesh = textComp.Data.ComputedMesh;
                },
                [this](size_t index)
                {
                    const auto& comp = m_TextComponents[index];
                    return comp.Data.Font && !comp.Data.Text.IsEmpty() && !comp.Data.ComputedMesh.GetVertexBuffer();
                }
            );

            job.Wait();
        }
        */
    }
}
