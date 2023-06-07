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

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 objectId = 0;
        for (auto& pair : computedBatchData.Batches)
        {
            // Update the draw command index
            /*
            pair.second.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                pair.second.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                pair.second.Count,
                0, 0, objectId
            };
            batchData.IndirectBuffer->SetElements(&command, 1, commandIndex);
            */

            // Should always be loaded & valid since CMB checks
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(
                data.Scene->GetMeshComponents()[pair.second.MeshIndex].Data.Mesh,
                true, async
            );
            auto& meshData = meshAsset->GetSubmesh(pair.second.SubmeshIndex);

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = computedBatchData.EntityListPool[pair.second.EntityListIndex];
            for (auto& entity : entityList)
            {
                const auto& entityData = data.Scene->GetEntityData()[entity.EntityIndex];
                const auto& materials = data.Scene->GetMeshComponents()[entity.MeshIndex].Data.Materials;

                auto selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(materials[meshData.GetMaterialIndex()]);
                if (materialAsset && materialAsset->IsValid())
                    selectedMaterial = &materialAsset->GetMaterial();

                bool batchesEmpty = newBatchData.Batches.IsEmpty();
                if (batchesEmpty || newBatchData.Batches.Back().Material != selectedMaterial)
                {
                    // Upload data for previous batch
                    if (!batchesEmpty)
                    {
                        auto& batch = newBatchData.Batches.Back();
                        
                        // Update the draw command index
                        batch.First = commandIndex;

                        // Popupate the indirect buffer
                        IndexedIndirectCommand command = {
                            meshData.GetIndexBuffer()->GetAllocatedCount(),
                            batch.Count,
                            0, 0, objectId
                        };
                        newBatchData.IndirectBuffer->SetElements(&command, 1, commandIndex);
                        
                        commandIndex++;
                    }

                    MaterialBatch batch = {
                        pair.second.Mesh,
                        selectedMaterial,
                        0, 0
                    };

                    newBatchData.Batches.AddInPlace(batch);
                }

                newBatchData.Batches.Back().Count++;
                newBatchData.RenderedInstanceCount++;

                // Object data
                batchData.ObjectDataBuffer->SetElements(&entityData.Transform, 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            // Only relevant when using GPU culling
            pair.second.Count = 1;

            commandIndex++;
        }
        
        batchData.RenderedObjectCount = objectId;
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
