#include "hepch.h"
#include "TLAS.h"

#include "glm/gtc/matrix_transform.hpp"
#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "glm/gtc/type_ptr.hpp"

namespace Heart::RenderPlugins
{
    void TLAS::Initialize()
    {
        Flourish::AccelerationStructureCreateInfo accelCreateInfo;
        accelCreateInfo.Type = Flourish::AccelerationStructureType::Scene;
        accelCreateInfo.PerformancePreference = Flourish::AccelerationStructurePerformanceType::FasterRuntime;
        accelCreateInfo.AllowUpdating = false;
        m_AccelStructure = Flourish::AccelerationStructure::Create(accelCreateInfo);
    }

    void TLAS::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::TLAS");

        m_Instances.Clear();

        Flourish::AccelerationStructureInstance instance;
        for (auto& meshComp : data.Scene->GetMeshComponents())
        {
            auto& entityData = data.Scene->GetEntityData()[meshComp.EntityIndex];

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(meshComp.Data.Mesh, true, true);
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);
                if (!meshData.GetAccelStructure()) continue;

                instance.Parent = meshData.GetAccelStructure();
                instance.TransformMatrix = glm::value_ptr(entityData.Transform);
                m_Instances.AddInPlace(instance);
            }
        }

        if (m_Instances.Count() > 0)
        {
            Flourish::AccelerationStructureSceneBuildInfo buildInfo;
            buildInfo.Instances = m_Instances.Data();
            buildInfo.InstanceCount = m_Instances.Count();
            m_AccelStructure->RebuildScene(buildInfo);
        }
        
    }
}
