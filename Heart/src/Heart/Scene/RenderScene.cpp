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
        auto srcText = scene->GetRegistry().view<TextComponent>();
        auto srcSplat = scene->GetRegistry().view<SplatComponent>();

        m_Registry.storage<entt::entity>().push(srcEntity.data(), srcEntity.data() + srcEntity.size());
        m_Registry.insert<MeshComponent>(srcMesh.begin(), srcMesh.end(), srcMesh.storage()->begin());
        m_Registry.insert<LightComponent>(srcLight.begin(), srcLight.end(), srcLight.storage()->begin());
        m_Registry.insert<TextComponent>(srcText.begin(), srcText.end(), srcText.storage()->begin());
        m_Registry.insert<SplatComponent>(srcSplat.begin(), srcSplat.end(), srcSplat.storage()->begin());

        m_CachedTransforms = scene->GetCachedTransforms();

        // Spawn jobs to recompute text data
        if (srcText.size() > 0)
        {
            auto dstText = m_Registry.view<TextComponent>();
            auto job = JobManager::ScheduleIter(
                dstText.begin(),
                dstText.end(),
                [dstText, scene](size_t index)
                {
                    auto& textComp = dstText.get<TextComponent>((entt::entity)index);
                    textComp.RecomputeRenderData();
                    
                    // Update original text component with new data so that we can cache the computed result
                    auto& ogTextComp = scene->GetRegistry().get<TextComponent>((entt::entity)index);
                    ogTextComp.ComputedMesh = textComp.ComputedMesh;
                },
                [dstText](size_t index)
                {
                    const auto& textComp = dstText.get<TextComponent>((entt::entity)index);
                    return textComp.Font && !textComp.Text.IsEmpty() && !textComp.ComputedMesh.GetVertexBuffer();
                }
            );

            job.Wait();
        }
    }
}
