#include "hepch.h"
#include "ComputeTextBatches.h"

#include "Flourish/Api/Buffer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/App.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/FontAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"
#include "Heart/Renderer/Plugins/CollectMaterials.h"

namespace Heart::RenderPlugins
{
    void ComputeTextBatches::InitializeInternal()
    {
        Flourish::BufferCreateInfo bufCreateInfo;

        for (u32 i = 0; i < Flourish::Context::FrameBufferCount(); i++)
        {
            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(ComputeMeshBatches::ObjectData);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_BatchData[i].ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Indirect;
            bufCreateInfo.Stride = sizeof(ComputeMeshBatches::IndexedIndirectCommand);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_BatchData[i].IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }

    void ComputeTextBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeTextBatches");

        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        const auto& materialMap = materialsPlugin->GetMaterialMap();
        
        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();

        bool async = data.Settings.AsyncAssetLoading;
        auto& newComputedData = m_BatchData[m_UpdateFrameIndex];
        
        // Clear previous data
        newComputedData.Batches.Clear();
        newComputedData.TotalInstanceCount = 0;

        // Populate text component batches
        u32 commandIndex = 0;
        u32 objectId = 0;
        auto textView = data.Scene->GetRegistry().view<TextComponent>();
        for (entt::entity entity : textView)
        {
            const auto& textComp = textView.get<TextComponent>(entity);

            if (!textComp.ComputedMesh.GetVertexBuffer())
                continue;

            const auto& transformData = data.Scene->GetCachedTransforms().at(entity);

            auto fontAsset = AssetManager::RetrieveAsset<FontAsset>(textComp.Font);
            if (!fontAsset || !fontAsset->Load(!async)->IsValid())
                continue;

            Material* selectedMaterial = nullptr;
            auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(textComp.Material);
            if (materialAsset && materialAsset->Load(!async)->IsValid())
                selectedMaterial = &materialAsset->GetMaterial();

            u64 materialId = (u64)selectedMaterial;
            if (materialMap.find(materialId) == materialMap.end())
                materialId = 0; // Default material

            // Here we assume each text component has a different mesh. This is likely the case, so
            // add a new batch each time
            TextBatch batch = {
                &textComp.ComputedMesh,
                selectedMaterial,
                fontAsset->GetAtlasTexture(),
                commandIndex,
                1
            };

            // Popupate the indirect buffer
            ComputeMeshBatches::IndexedIndirectCommand command = {
                batch.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                batch.Count,
                0, 0, objectId
            };
            newComputedData.IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Change the count to represent the number of draw commands
            batch.Count = 1;
            newComputedData.Batches.AddInPlace(batch);

            commandIndex++;
            newComputedData.TotalInstanceCount++;

            // Object data
            ComputeMeshBatches::ObjectData objectData = {
                transformData.Transform,
                materialMap.at(materialId),
                (u32)entity
            };
            newComputedData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);
            objectId++;
        }

        m_Stats["Instance Count"] = {
            StatType::Int,
            (int)newComputedData.TotalInstanceCount
        };
        m_Stats["Batch Count"] = {
            StatType::Int,
            (int)newComputedData.Batches.Count()
        };
    }
}
