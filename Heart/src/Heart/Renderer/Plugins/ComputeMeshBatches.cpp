#include "hepch.h"
#include "ComputeMeshBatches.h"

#include "Heart/Renderer/SceneRenderer2.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/App.h"

namespace Heart::RenderPlugins
{
    void ComputeMeshBatches::RenderInternal(const SceneRenderData& data, SceneRenderer2* sceneRenderer)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeMeshBatches");
        
        // TODO: revisit this. Disabling previous-frame batch rendering for now because there is a lot of nuance in terms of
        // ghosting, culling, etc. that really isn't worth it right now
        m_UpdateFrameIndex = App::Get().GetFrameCount() % Flourish::Context::FrameBufferCount();
        m_RenderFrameIndex = m_UpdateFrameIndex;//(App::Get().GetFrameCount() - 1) % Flourish::Context::FrameBufferCount();

        bool async = data.Settings.AsyncAssetLoading;
        auto& batchData = m_BatchData[m_UpdateFrameIndex];

        // Clear previous data
        batchData.Batches.clear();
        batchData.RenderedInstanceCount = 0;
        batchData.RenderedObjectCount = 0;
        for (auto& list : batchData.EntityListPool)
            list.Clear();
        
        // TODO: check for transparent/translucent stuff
        
        // Loop over each mesh component / submesh, hash the mesh & material, and place the entity in a batch
        // associated with the mesh & material. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        for (u32 meshCompIndex = 0; meshCompIndex < data.Scene->GetMeshComponents().Count(); meshCompIndex++)
        {
            const auto& meshComp = data.Scene->GetMeshComponents()[meshCompIndex];
            const auto& entityData = data.Scene->GetEntityData()[meshComp.EntityIndex];

            // Compute max scale for calculating the bounding sphere
            // TODO: scale scale by some factor or some sort of predictive culling based on camera speed
            glm::vec3 scale = entityData.Scale;
            float maxScale = std::max(std::max(scale.x, scale.y), scale.z);

            // Skip invalid meshes
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(meshComp.Data.Mesh, true, async);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
               
                glm::vec4 boundingSphere = meshData.GetBoundingSphere();
                boundingSphere.w *= maxScale; // Extend the bounding sphere to fit the largest scale 
                if (data.Settings.CullEnable && 
                    !FrustumCull(data, boundingSphere, entityData.Transform))
                    continue;
                
                // Create a hash based on the submesh
                u64 hash = (meshComp.Data.Mesh + i) ^ (i * 45787893);

                // Get/create a batch associated with this hash
                auto& batch = batchData.Batches[hash];

                // Update the batch information if this is the first entity being added to it
                if (batch.Count == 0)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= batchData.EntityListPool.Count())
                        batchData.EntityListPool.AddInPlace();

                    // Set the mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.MeshIndex = meshCompIndex;
                    batch.SubmeshIndex = i;

                    batchIndex++;
                }
                
                batch.Count++;
                batchData.RenderedInstanceCount++;

                // Push the associated entity to the associated vector from the pool
                batchData.EntityListPool[batch.EntityListIndex].AddInPlace(
                    static_cast<u32>(meshComp.EntityIndex),
                    meshCompIndex
                );
            }
        }

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 objectId = 0;
        for (auto& pair : batchData.Batches)
        {
            // Update the draw command index
            pair.second.First = commandIndex;

            // Popupate the indirect buffer
            IndexedIndirectCommand command = {
                pair.second.Mesh->GetIndexBuffer()->GetAllocatedCount(),
                pair.second.Count,
                0, 0, objectId
            };
            batchData.IndirectBuffer->SetElements(&command, 1, commandIndex);

            // Contiguiously set the instance data for each entity associated with this batch
            auto& entityList = batchData.EntityListPool[pair.second.EntityListIndex];
            for (auto& entity : entityList)
            {
                const auto& entityData = data.Scene->GetEntityData()[entity.EntityIndex];

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

    void ComputeMeshBatches::Initialize()
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
            bufCreateInfo.Type = Flourish::BufferType::Indirect;
            bufCreateInfo.Stride = sizeof(IndexedIndirectCommand);
            bufCreateInfo.ElementCount = m_MaxObjects;
            m_BatchData[i].IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        }
    }

    bool ComputeMeshBatches::FrustumCull(const SceneRenderData& data, glm::vec4 boundingSphere, const glm::mat4& transform)
    {
        float radius = boundingSphere.w;
        boundingSphere.w = 1.f;
        glm::vec3 center = transform * boundingSphere;
        for (int i = 0; i < 6; i++)
        {
            auto& plane = data.Camera->GetFrustumPlanes()[i];
            if (plane.x * center.x + plane.y * center.y + plane.z * center.z + plane.w <= -radius)
                return false;
        }
        
        return true;
    }
}
