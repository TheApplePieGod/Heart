#include "hepch.h"
#include "TLAS.h"

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
#include "Heart/Renderer/Plugins/CollectMaterials.h"
#include "glm/gtc/type_ptr.hpp"

namespace Heart::RenderPlugins
{
    void TLAS::Initialize()
    {
        Flourish::AccelerationStructureCreateInfo accelCreateInfo;
        accelCreateInfo.Type = Flourish::AccelerationStructureType::Scene;
        accelCreateInfo.PerformancePreference = Flourish::AccelerationStructurePerformanceType::FasterRuntime;
        accelCreateInfo.BuildFrequency = Flourish::AccelerationStructureBuildFrequency::Often;
        accelCreateInfo.AllowUpdating = false;
        m_AccelStructure = Flourish::AccelerationStructure::Create(accelCreateInfo);

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(ObjectData);
        bufCreateInfo.ElementCount = m_MaxObjects;
        m_ObjectBuffer = Flourish::Buffer::Create(bufCreateInfo);

        Flourish::CommandBufferCreateInfo cbCreateInfo;
        cbCreateInfo.FrameRestricted = true;
        cbCreateInfo.DebugName = m_Name.Data();
        m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

        m_GPUGraphNodeBuilder.Reset()
            .SetCommandBuffer(m_CommandBuffer.get())
            .AddEncoderNode(Flourish::GPUWorkloadType::Compute);
            // .AccelStructure ???
    }

    void TLAS::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::TLAS");

        auto materialsPlugin = m_Renderer->GetPlugin<RenderPlugins::CollectMaterials>(m_Info.CollectMaterialsPluginName);
        const auto& materialMap = materialsPlugin->GetMaterialMap();

        m_Instances.Clear();

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

                u64 materialId = (u64)selectedMaterial;
                if (materialMap.find(materialId) == materialMap.end())
                    materialId = 0; // Default material

                ObjectData objectData {
                    meshData.GetVertexBuffer()->GetBufferGPUAddress(),
                    meshData.GetIndexBuffer()->GetBufferGPUAddress(),
                    glm::vec4(materialMap.at(materialId))
                };
                m_ObjectBuffer->SetElements(&objectData, 1, m_Instances.Count());

                instance.Parent = meshData.GetAccelStructure();
                instance.TransformMatrix = glm::value_ptr(entityData.Transform);
                m_Instances.AddInPlace(instance);
            }
        }

        auto encoder = m_CommandBuffer->EncodeComputeCommands();
        Flourish::AccelerationStructureSceneBuildInfo buildInfo;
        buildInfo.Instances = m_Instances.Data();
        buildInfo.InstanceCount = m_Instances.Count();
        encoder->RebuildAccelerationStructureScene(m_AccelStructure.get(), buildInfo);
        encoder->EndEncoding();
    }
}
