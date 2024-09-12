#include "hepch.h"
#include "ComputeMeshBatches.h"

#include "Heart/Renderer/Plugins/CollectMaterials.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Core/Timing.h"
#include "Heart/Core/App.h"

namespace Heart::RenderPlugins
{
    void ComputeMeshBatches::InitializeInternal()
    {
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPUWriteFrame;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Storage;
        bufCreateInfo.Stride = sizeof(ObjectData);
        bufCreateInfo.ElementCount = m_MaxObjects;
        m_BatchData.ObjectDataBuffer = Flourish::Buffer::Create(bufCreateInfo);

        bufCreateInfo.MemoryType = Flourish::BufferMemoryType::CPUWriteFrame;
        bufCreateInfo.Usage = Flourish::BufferUsageFlags::Indirect;
        bufCreateInfo.Stride = sizeof(IndexedIndirectCommand);
        bufCreateInfo.ElementCount = m_MaxObjects;
        m_BatchData.IndirectBuffer = Flourish::Buffer::Create(bufCreateInfo);
    }

    void ComputeMeshBatches::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::ComputeMeshBatches");

        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        const auto& materialMap = materialsPlugin->GetMaterialMap();

        bool async = data.Settings.AsyncAssetLoading;
        auto& batchData = m_BatchData;

        // Clear previous data
        batchData.Batches.clear();
        batchData.TotalInstanceCount = 0;
        for (auto& list : batchData.EntityListPool)
            list.Clear();
        
        // Loop over each mesh component / submesh, hash the mesh, and place the entity in a batch
        // associated with the mesh. At this stage, Batch.First is unused and Batch.Count indicates
        // how many instances there are
        u32 batchIndex = 0;
        auto meshView = data.Scene->GetRegistry().view<MeshComponent>();
        for (entt::entity entity : meshView)
        {
            const auto& meshComp = meshView.get<MeshComponent>(entity);
            const auto& transformData = data.Scene->GetCachedTransforms().at(entity);

            // Compute max scale for calculating the bounding sphere
            // TODO: scale scale by some factor or some sort of predictive culling based on camera speed
            glm::vec3 scale = transformData.Scale;
            float maxScale = std::max(std::max(scale.x, scale.y), scale.z);

            // Skip invalid meshes
            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(meshComp.Mesh);
            if (!meshAsset || !meshAsset->Load(!async)->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
               
                glm::vec4 boundingSphere = meshData.GetBoundingSphere();
                boundingSphere.w *= maxScale; // Extend the bounding sphere to fit the largest scale 
                if (data.Settings.CullEnable && 
                    !FrustumCull(data, boundingSphere, transformData.Transform))
                    continue;
                
                // Create a hash based on the submesh
                u64 hash = (meshComp.Mesh + i) ^ (i * 45787893);

                // Get/create a batch associated with this hash
                auto& batch = batchData.Batches[hash];

                // Update the batch information if this is the first entity being added to it
                if (!batch.Mesh)
                {
                    // Retrieve a vector from the pool
                    batch.EntityListIndex = batchIndex;
                    if (batchIndex >= batchData.EntityListPool.Count())
                        batchData.EntityListPool.AddInPlace();

                    // Set the mesh associated with this batch
                    batch.Mesh = &meshData;
                    batch.MeshIndex = (u32)entity;
                    batch.SubmeshIndex = i;

                    batchIndex++;
                }
                
                // Need to check if the entity supports depth prepass before adding
                auto selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()];
                if (meshData.GetMaterialIndex() < meshComp.Materials.Count())
                {
                    auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(
                        meshComp.Materials[meshData.GetMaterialIndex()]
                    );

                    if (materialAsset && materialAsset->Load(!async)->IsValid())
                        selectedMaterial = &materialAsset->GetMaterial();
                }
                bool usePrepass = selectedMaterial->GetTransparencyMode() != TransparencyMode::AlphaBlend;

                u64 materialId = (u64)selectedMaterial;
                if (materialMap.find(materialId) == materialMap.end())
                    materialId = 0; // Default material

                if (usePrepass)
                {
                    batch.Count++;
                    batchData.TotalInstanceCount++;
                }

                // Push the associated entity to the associated vector from the pool
                batchData.EntityListPool[batch.EntityListIndex].AddInPlace(EntityListEntry {
                    (u32)entity,
                    materialMap.at(materialId),
                    usePrepass
                });
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
                if (!entity.IncludeInPrepass) continue;

                const auto& transformData = data.Scene->GetCachedTransforms().at((entt::entity)entity.EntityId);

                // Object data
                ObjectData objectData = {
                    transformData.Transform,
                    entity.MaterialIndex,
                    entity.EntityId
                };
                batchData.ObjectDataBuffer->SetElements(&objectData, 1, objectId);

                objectId++;
            }

            // Change the count to represent the number of draw commands
            // Only relevant when using GPU culling
            pair.second.Count = 1;

            commandIndex++;
        }
        
        m_Stats["Instance Count"] = {
            StatType::Int,
            (int)batchData.TotalInstanceCount
        };
        m_Stats["Batch Count"] = {
            StatType::Int,
            (int)batchData.Batches.size()
        };
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
