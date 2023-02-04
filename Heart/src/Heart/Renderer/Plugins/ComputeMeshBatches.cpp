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
        auto& batchData = m_BatchRenderData[m_UpdateFrameIndex];
        
        // Loop over each mesh component / submesh, hash the mesh & material, and place the entity in a batch
        // associated with the mesh & material. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        for (const auto& meshComp : data.Scene->GetMeshComponents())
        {
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
                
                // Create a hash based on the submesh and its material if applicable
                u64 hash = meshComp.Data.Mesh ^ (i * 45787893);
                if (meshData.GetMaterialIndex() < meshComp.Data.Materials.Count())
                    hash ^= meshComp.Data.Materials[meshData.GetMaterialIndex()];

                // Get/create a batch associated with this hash
                auto& batch = batchData.IndirectBatches[hash];

                // Update the batch information if this is the first entity being added to it
                if (batch.Count == 0)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= batchData.EntityListPool.Count())
                        batchData.EntityListPool.AddInPlace();

                    // Set the material & mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.Material = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()]; // default material
                    if (meshComp.Data.Materials.Count() > meshData.GetMaterialIndex())
                    {
                        auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(meshComp.Data.Materials[meshData.GetMaterialIndex()]);
                        if (materialAsset && materialAsset->IsValid())
                            batch.Material = &materialAsset->GetMaterial();
                    }

                    batchIndex++;
                }
                
                batch.Count++;
                batchData.RenderedInstanceCount++;

                // Push the associated entity to the associated vector from the pool
                batchData.EntityListPool[batch.EntityListIndex].AddInPlace(static_cast<u32>(meshComp.EntityIndex));
            }
        }

        // Loop over the calculated batches and populate the indirect buffer with draw commands. Because we are instancing, we need to make sure each object/material data
        // element gets placed contiguously for each indirect draw call. At this stage, Batch.First is the index of the indirect draw command in the buffer and
        // Batch.Count will equal 1 because it represents how many draw commands are in each batch
        u32 commandIndex = 0;
        u32 objectId = 0;
        for (auto& pair : batchData.IndirectBatches)
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
            for (u32 entity : entityList)
            {
                const auto& entityData = m_Scene->GetEntityData()[entity];

                // Object data
                ObjectData objectData = {
                    entityData.Transform,
                    { entityData.Id, 0.f, 0.f, 0.f }
                };
                batchData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                // Material data
                if (pair.second.Material)
                {
                    auto& materialData = pair.second.Material->GetMaterialData();

                    auto albedoAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetAlbedoTexture(), true, async);
                    materialData.SetHasAlbedo(albedoAsset && albedoAsset->IsValid());

                    auto metallicRoughnessAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetMetallicRoughnessTexture(), true, async);
                    materialData.SetHasMetallicRoughness(metallicRoughnessAsset && metallicRoughnessAsset->IsValid());

                    auto normalAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetNormalTexture(), true, async);
                    materialData.SetHasNormal(normalAsset && normalAsset->IsValid());

                    auto emissiveAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetEmissiveTexture(), true, async);
                    materialData.SetHasEmissive(emissiveAsset && emissiveAsset->IsValid());

                    auto occlusionAsset = AssetManager::RetrieveAsset<TextureAsset>(pair.second.Material->GetOcclusionTexture(), true, async);
                    materialData.SetHasOcclusion(occlusionAsset && occlusionAsset->IsValid());

                    batchData.MaterialDataBuffer->SetElements(&materialData, 1, objectId);
                }
                else
                    batchData.MaterialDataBuffer->SetElements(&AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true)->GetMaterial().GetMaterialData(), 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            pair.second.Count = 1;

            commandIndex++;
        }
        
        batchData.RenderedObjectCount = objectId;
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