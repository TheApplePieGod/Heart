#include "hepch.h"
#include "ComputeMaterialBatches.h"

#include "Flourish/Api/Buffer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/App.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Renderer/SceneRenderer2.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"

namespace Heart::RenderPlugins
{
    void ComputeMaterialBatches::RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeMeshBatches");
        
        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();

        bool async = data.Settings.AsyncAssetLoading;
        auto& newBatchData = m_BatchData[m_UpdateFrameIndex];
        auto batchesPlugin = sceneRenderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName);
        const auto& computedBatchData = batchesPlugin->GetBatchData();
        
        // Clear previous data
        newBatchData.Batches.Clear();
        newBatchData.RenderedInstanceCount = 0;
        newBatchData.RenderedObjectCount = 0;

        u32 commandIndex = 0;
        u32 objectId = 0;
        u32 objectIdStart = 0;
        auto addIndirectCommand = [](const Mesh& meshData, BatchData& batchData, u32 commandIndex, u32 objectId)
        {
            auto& batch = batchData.Batches.Back();
            
            // Update the draw command index
            batch.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                meshData.GetIndexBuffer()->GetAllocatedCount(),
                batch.Count,
                0, 0, objectId
            };
            batchData.IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Change the count to represent the number of draw commands
            batch.Count = 1;
        };

        // Using the previously computed mesh batches, compute new batches that group by material. Since we do not do a full recomputation,
        // each batch is subdivided first by mesh and then by material.
        for (auto& pair : computedBatchData.Batches)
        {
            // Should always be loaded & valid since CMB checks
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(
                data.Scene->GetMeshComponents()[pair.second.MeshIndex].Data.Mesh,
                true, async
            );
            auto& meshData = meshAsset->GetSubmesh(pair.second.SubmeshIndex);

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = computedBatchData.EntityListPool[pair.second.EntityListIndex];
            for (u32 entityListIndex = 0; entityListIndex < entityList.Count(); entityListIndex++)
            {
                auto& entity = entityList[entityListIndex];
                const auto& entityData = data.Scene->GetEntityData()[entity.EntityIndex];
                const auto& materials = data.Scene->GetMeshComponents()[entity.MeshIndex].Data.Materials;

                auto selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(materials[meshData.GetMaterialIndex()]);
                if (materialAsset && materialAsset->IsValid())
                    selectedMaterial = &materialAsset->GetMaterial();

                bool batchesEmpty = newBatchData.Batches.IsEmpty();
                if (batchesEmpty || newBatchData.Batches.Back().Material != selectedMaterial)
                {
                    if (!batchesEmpty)
                    {
                        addIndirectCommand(meshData, newBatchData, commandIndex, objectIdStart);
                        objectIdStart = objectId;
                        commandIndex++;
                    }

                    // Populate the material buffer
                    newBatchData.MaterialDataBuffer->SetElements(&selectedMaterial->GetMaterialData(), 1, commandIndex);
                    
                    MaterialBatch batch = {
                        pair.second.Mesh,
                        selectedMaterial,
                        commandIndex,
                        0
                    };

                    newBatchData.Batches.AddInPlace(batch);
                }

                newBatchData.Batches.Back().Count++;
                newBatchData.RenderedInstanceCount++;

                // Object data
                ObjectData objectData = {
                    entityData.Transform,
                    { entityData.Id, commandIndex, 0.f, 0.f }
                };
                newBatchData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                // Always add on the last elem
                if (entityListIndex == entityList.Count() - 1)
                    addIndirectCommand(meshData, newBatchData, commandIndex, objectIdStart);

                objectId++;
            }
        }
        
        newBatchData.RenderedObjectCount = objectId;
    }
    
    void ComputeMaterialBatches::Initialize()
    {
        Flourish::BufferCreateInfo bufCreateInfo;

        for (u32 i = 0; i < Flourish::Context::FrameBufferCount(); i++)
        {
            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(ObjectData);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_BatchData[i].ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(MaterialData);
            bufCreateInfo.ElementCount = m_MaxMaterials;
            m_BatchData[i].MaterialDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Indirect;
            bufCreateInfo.Stride = sizeof(IndexedIndirectCommand);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_BatchData[i].IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }
}
