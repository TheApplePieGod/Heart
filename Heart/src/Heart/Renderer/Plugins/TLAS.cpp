#include "hepch.h"
#include "TLAS.h"

#include "glm/gtc/matrix_transform.hpp"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ResourceSetAllocator.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Renderer/Material.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "glm/gtc/type_ptr.hpp"

#include "Flourish/Backends/Vulkan/Buffer.h"

namespace Heart::RenderPlugins
{
    void TLAS::Initialize()
    {
        Flourish::AccelerationStructureCreateInfo accelCreateInfo;
        accelCreateInfo.Type = Flourish::AccelerationStructureType::Scene;
        accelCreateInfo.PerformancePreference = Flourish::AccelerationStructurePerformanceType::FasterRuntime;
        accelCreateInfo.BuildFrequency = Flourish::AccelerationStructureBuildFrequency::PerFrame;
        accelCreateInfo.AllowUpdating = false;
        m_AccelStructure = Flourish::AccelerationStructure::Create(accelCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(ObjectData);
        bufCreateInfo.ElementCount = m_MaxObjects;
        m_ObjectBuffer = Flourish::Buffer::Create(bufCreateInfo);
        
        // TODO: copying all the materials is mid
        bufCreateInfo.Stride = sizeof(MaterialData);
        bufCreateInfo.ElementCount = m_MaxObjects;
        m_MaterialBuffer = Flourish::Buffer::Create(bufCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        Flourish::ResourceSetAllocatorCreateInfo setAllocCreateInfo;
        setAllocCreateInfo.Compatability = Flourish::ResourceSetPipelineCompatabilityFlags::RayTracing;
        setAllocCreateInfo.Layout = {
            {
                0,
                Flourish::ShaderResourceType::Texture,
                Flourish::ShaderTypeFlags::RayClosestHit,
                5000
            }
        };
        auto setAllocator = Flourish::ResourceSetAllocator::Create(setAllocCreateInfo);
        Flourish::ResourceSetCreateInfo setCreateInfo;
        setCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_TexturesSet = setAllocator->Allocate(setCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute);
            // .AccelStructure ???
    }

    void TLAS::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::TLAS");

        m_Instances.Clear();

        const auto& defaultTex = AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture();
        auto bindTex = [&](UUID texId, u32 arrayIndex)
        {
            auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(texId);
            bool valid = texAsset && texAsset->IsValid();
            if (valid)
                m_TexturesSet->BindTexture(0, texAsset->GetTexture(), arrayIndex);
            else
                m_TexturesSet->BindTexture(0, defaultTex, arrayIndex);
        };

        Flourish::AccelerationStructureInstance instance;
        for (auto& meshComp : data.Scene->GetMeshComponents())
        {
            auto& entityData = data.Scene->GetEntityData()[meshComp.EntityIndex];

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(
                meshComp.Data.Mesh,
                true,
                data.Settings.AsyncAssetLoading
            );
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
                if (!meshData.GetAccelStructure()) continue;

                ObjectData objectData {
                    meshData.GetVertexBuffer()->GetBufferGPUAddress(),
                    meshData.GetIndexBuffer()->GetBufferGPUAddress(),
                };
                m_ObjectBuffer->SetElements(&objectData, 1, m_Instances.Count());

                auto selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()];
                if (meshData.GetMaterialIndex() < meshComp.Data.Materials.Count())
                {
                    auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(
                        meshComp.Data.Materials[meshData.GetMaterialIndex()],
                        true,
                        data.Settings.AsyncAssetLoading
                    );
                    if (materialAsset && materialAsset->IsValid())
                        selectedMaterial = &materialAsset->GetMaterial();
                }
                m_MaterialBuffer->SetElements(&selectedMaterial->GetMaterialData(), 1, m_Instances.Count());

                u32 baseIndex = m_Instances.Count() * 5;
                bindTex(selectedMaterial->GetAlbedoTexture(), baseIndex);
                bindTex(selectedMaterial->GetMetallicRoughnessTexture(), baseIndex + 1);
                bindTex(selectedMaterial->GetNormalTexture(), baseIndex + 2);
                bindTex(selectedMaterial->GetEmissiveTexture(), baseIndex + 3);
                bindTex(selectedMaterial->GetOcclusionTexture(), baseIndex + 4);

                instance.Parent = meshData.GetAccelStructure();
                instance.TransformMatrix = glm::value_ptr(entityData.Transform);
                instance.CustomIndex = Flourish::Context::FrameCount();
                m_Instances.AddInPlace(instance);
            }
        }

        m_TexturesSet->FlushBindings();

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        Flourish::AccelerationStructureSceneBuildInfo buildInfo;
        buildInfo.Instances = m_Instances.Data();
        buildInfo.InstanceCount = m_Instances.Count();
        encoder->RebuildAccelerationStructureScene(m_AccelStructure.get(), buildInfo);
        encoder->EndEncoding();
    }
}
