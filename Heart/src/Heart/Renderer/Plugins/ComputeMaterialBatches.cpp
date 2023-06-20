#include "hepch.h"
#include "ComputeMaterialBatches.h"

#include "Flourish/Api/Buffer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/App.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Renderer/Plugins/ComputeMeshBatches.h"

namespace Heart::RenderPlugins
{
    void ComputeMaterialBatches::BatchData::Clear()
    {
        Batches.Clear();
        TotalInstanceCount = 0;
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
            m_ComputedData[i].ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Stride = sizeof(MaterialData);
            bufCreateInfo.ElementCount = m_MaxMaterials;
            m_ComputedData[i].MaterialDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

            bufCreateInfo.Usage = Flourish::BufferUsageType::DynamicOneFrame;
            bufCreateInfo.Type = Flourish::BufferType::Indirect;
            bufCreateInfo.Stride = sizeof(IndexedIndirectCommand);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_ComputedData[i].IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }

    // TODO: this could be potentially split up into multiple modules
    void ComputeMaterialBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeMaterialBatches");
        
        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();

        bool async = data.Settings.AsyncAssetLoading;
        auto& newComputedData = m_ComputedData[m_UpdateFrameIndex];
        auto batchesPlugin = m_Renderer->GetPlugin<RenderPlugins::ComputeMeshBatches>(m_Info.MeshBatchesPluginName);
        const auto& computedBatchData = batchesPlugin->GetBatchData();
        
        // Clear previous data
        for (auto& batch : newComputedData.BatchTypes)
            batch.Clear();

        u32 commandIndex = 0;
        u32 materialIndex = 0;
        u32 objectId = 0;
        u32 objectIdStart = 0;
        auto addIndirectCommand = [](ComputedData& computedData, u32& commandIndex, u32 objectId, MaterialBatch& batch)
        {
            // Update the draw command index
            batch.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                batch.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                batch.Count,
                0, 0, objectId
            };
            computedData.IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Change the count to represent the number of draw commands
            batch.Count = 1;

            commandIndex++;
        };

        // Using the previously computed mesh batches, compute new batches that group by material. Since we do not do a full recomputation,
        // each batch is subdivided first by mesh and then by material.
        u32 pairIndex = 0;
        HVector<MaterialBatch>* oldBatches = nullptr;
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
                if (meshData.GetMaterialIndex() < materials.Count()) // This check may not be necessary if data is saved correctly
                {
                    auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(materials[meshData.GetMaterialIndex()]);
                    if (materialAsset && materialAsset->IsValid())
                        selectedMaterial = &materialAsset->GetMaterial();
                }

                bool isAlpha = selectedMaterial->GetTransparencyMode() == TransparencyMode::AlphaBlend;
                auto& batchData = newComputedData.BatchTypes[(u32)(isAlpha ? BatchType::Alpha : BatchType::Opaque)];

                bool batchSwitch = oldBatches != &batchData.Batches;
                bool materialSwitch = !batchSwitch && batchData.Batches.Back().Material != selectedMaterial;
                if (batchSwitch || materialSwitch)
                {
                    if (batchSwitch && oldBatches && !oldBatches->IsEmpty())
                    {
                        addIndirectCommand(newComputedData, commandIndex, objectIdStart, oldBatches->Back());
                        objectIdStart = objectId;
                    }
                    if (materialSwitch && !batchData.Batches.IsEmpty())
                    {
                        addIndirectCommand(newComputedData, commandIndex, objectIdStart, batchData.Batches.Back());
                        objectIdStart = objectId;
                    }

                    // Populate the material buffer
                    materialIndex++;
                    newComputedData.MaterialDataBuffer->SetElements(&selectedMaterial->GetMaterialData(), 1, materialIndex);
                    
                    MaterialBatch batch = {
                        pair.second.Mesh,
                        selectedMaterial,
                        commandIndex,
                        0
                    };

                    batchData.Batches.AddInPlace(batch);
                }

                batchData.Batches.Back().Count++;
                batchData.TotalInstanceCount++;

                // Object data
                ObjectData objectData = {
                    entityData.Transform,
                    { entityData.Id, materialIndex, 0.f, 0.f }
                };
                newComputedData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                objectId++;
                oldBatches = &batchData.Batches;
            }

            // Always add on the last elem of the last mesh
            if (pairIndex == computedBatchData.Batches.size() - 1)
                addIndirectCommand(newComputedData, commandIndex, objectIdStart, oldBatches->Back());

            pairIndex++;
        }

        newComputedData.TotalInstanceCount = 0;
        for (auto& batch : newComputedData.BatchTypes)
            newComputedData.TotalInstanceCount += batch.TotalInstanceCount;

        m_Stats["Instance Count"] = {
            StatType::Int,
            (int)newComputedData.TotalInstanceCount
        };
        /*
        m_Stats["Batch Count"] = {
            StatType::Int,
            (int)newComputedData.Batches.Count()
        };
        */
    }
}
