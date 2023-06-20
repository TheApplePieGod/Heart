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

namespace Heart::RenderPlugins
{
    void ComputeTextBatches::Initialize()
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
    void ComputeTextBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeTextBatches");
        
        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();

        bool async = data.Settings.AsyncAssetLoading;
        auto& newComputedData = m_ComputedData[m_UpdateFrameIndex];
        
        // Clear previous data
        newComputedData.Batches.Clear();
        newComputedData.TotalInstanceCount = 0;

        // Populate text component batches
        u32 commandIndex = 0;
        u32 materialIndex = 0;
        u32 objectId = 0;
        auto& defaultMaterial = AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true)->GetMaterial();
        for (u32 textCompIndex = 0; textCompIndex < data.Scene->GetTextComponents().Count(); textCompIndex++)
        {
            auto& textComp = data.Scene->GetTextComponents()[textCompIndex];
            const auto& entityData = data.Scene->GetEntityData()[textComp.EntityIndex];

            if (!textComp.Data.ComputedMesh.GetVertexBuffer())
                continue;

            auto fontAsset = AssetManager::RetrieveAsset<FontAsset>(textComp.Data.Font);
            if (!fontAsset || !fontAsset->IsValid())
                continue;

            Material* selectedMaterial = &defaultMaterial;
            auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(textComp.Data.Material);
            if (materialAsset && materialAsset->IsValid())
                selectedMaterial = &materialAsset->GetMaterial();

            // Here we assume each text component has a different mesh. This is likely the case, so
            // add a new batch each time
            TextBatch batch = {
                &textComp.Data.ComputedMesh,
                selectedMaterial,
                fontAsset->GetAtlasTexture(),
                commandIndex,
                1
            };

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
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

            bool materialSwitch = textCompIndex == 0 || newComputedData.Batches.Back().Material != selectedMaterial;
            if (materialSwitch)
            {
                // Populate the material buffer
                materialIndex++;
                newComputedData.MaterialDataBuffer->SetElements(&selectedMaterial->GetMaterialData(), 1, materialIndex);
            }

            // Object data
            ObjectData objectData = {
                entityData.Transform,
                { entityData.Id, materialIndex, 0.f, 0.f }
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
