#include "hepch.h"
#include "LightingData.h"

#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/RenderGraph.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/TransferCommandEncoder.h"
#include "Heart/Core/Timing.h"
#include "Heart/Scene/Components.h"
#include "Heart/Renderer/SceneRenderer.h"
#include "glm/gtc/type_ptr.hpp"

namespace Heart::RenderPlugins
{
    // TODO: could separate lighting accel structure into own plugin
    void LightingData::InitializeInternal()
    {
        m_UseRayTracing = Flourish::Context::FeatureTable().RayTracing;

        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(LightData);
        bufCreateInfo.ElementCount = m_MaxLights;
        m_Buffer = Flourish::Buffer::Create(bufCreateInfo);
        bufCreateInfo.Stride = sizeof(glm::vec4);
        bufCreateInfo.ElementCount = m_MaxDirectionalLights / 4;
        m_DirectionalBuffer = Flourish::Buffer::Create(bufCreateInfo);

        if (m_UseRayTracing)
        {
            Flourish::CommandBufferCreateInfo cbCreateInfo;
            cbCreateInfo.FrameRestricted = true;
            cbCreateInfo.DebugName = m_Name.Data();
            m_CommandBuffer = Flourish::CommandBuffer::Create(cbCreateInfo);

            m_GPUGraphNodeBuilder.Reset()
                .SetCommandBuffer(m_CommandBuffer.get())
                .AddEncoderNode(Flourish::GPUWorkloadType::Compute);
                // .AccelStructure ???

            Flourish::AccelerationStructureCreateInfo accelCreateInfo;
            accelCreateInfo.Type = Flourish::AccelerationStructureType::Node;
            accelCreateInfo.PerformancePreference = Flourish::AccelerationStructurePerformanceType::FasterRuntime;
            accelCreateInfo.BuildFrequency = Flourish::AccelerationStructureBuildFrequency::Once;
            accelCreateInfo.AllowUpdating = false;
            m_LightBLAS = Flourish::AccelerationStructure::Create(accelCreateInfo);
            accelCreateInfo.Type = Flourish::AccelerationStructureType::Scene;
            accelCreateInfo.BuildFrequency = Flourish::AccelerationStructureBuildFrequency::Often;
            m_LightTLAS = Flourish::AccelerationStructure::Create(accelCreateInfo);

            /*
             * Build BLAS immediately
             */

            // Allocate one-time graph and cmd buf
            Ref<Flourish::CommandBuffer> uploadBuf = Flourish::CommandBuffer::Create({ false });
            Ref<Flourish::RenderGraph> graph = Flourish::RenderGraph::Create({ Flourish::RenderGraphUsageType::Once });

            // Allocate the buffer storing the unit AABB
            std::array<float, 6> unitAABB = { -1.f, -1.f, -1.f, 1.f, 1.f, 1.f };
            auto uploadEncoder = uploadBuf->EncodeTransferCommands();
            Flourish::BufferCreateInfo bufCreateInfo;
            bufCreateInfo.Type = Flourish::BufferType::Storage;
            bufCreateInfo.Usage = Flourish::BufferUsageType::Static;
            bufCreateInfo.Stride = sizeof(unitAABB);
            bufCreateInfo.ElementCount = 1;
            bufCreateInfo.InitialData = unitAABB.data();
            bufCreateInfo.InitialDataSize = sizeof(unitAABB);
            bufCreateInfo.UploadEncoder = uploadEncoder;
            bufCreateInfo.CanCreateAccelerationStructure = true;
            bufCreateInfo.ExposeGPUAddress = true;
            auto aabbBuffer = Flourish::Buffer::Create(bufCreateInfo);
            uploadEncoder->EndEncoding();

            auto builder = graph->ConstructNewNode(uploadBuf.get());
            builder.AddEncoderNode(Flourish::GPUWorkloadType::Transfer)
                .EncoderAddBufferWrite(aabbBuffer.get())
                .AddEncoderNode(Flourish::GPUWorkloadType::Compute)
                .EncoderAddBufferRead(aabbBuffer.get())
                .AddToGraph();
            graph->Build();

            Flourish::AccelerationStructureNodeBuildInfo buildInfo;
            buildInfo.AABBBuffer = aabbBuffer.get();
            auto buildEncoder = uploadBuf->EncodeComputeCommands();
            buildEncoder->RebuildAccelerationStructureNode(m_LightBLAS.get(), buildInfo);
            buildEncoder->EndEncoding();

            Flourish::Context::ExecuteRenderGraph(graph.get());
        }
    }

    void LightingData::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::LightingData");

        m_Instances.Clear();
        m_Transforms.Clear();

        Flourish::AccelerationStructureInstance instance;
        instance.Parent = m_LightBLAS.get();

        u32 lightIndex = 1;
        u32 directionalIndex = 1;
        auto lightView = data.Scene->GetRegistry().view<LightComponent>();
        for (entt::entity entity : lightView)
        {
            if (lightIndex >= m_MaxLights)
                break;

            const auto& lightComp = lightView.get<LightComponent>(entity);
            const auto& transformData = data.Scene->GetCachedTransforms().at(entity);

            u32 offset = lightIndex * m_Buffer->GetStride();

            if (lightComp.LightType == LightComponent::Type::Disabled) continue;

            // Update the translation part of the light struct
            m_Buffer->SetBytes(&transformData.Position, sizeof(glm::vec3), offset);
            offset += sizeof(glm::vec4);

            // Update the light direction if the light is not a point light
            if (lightComp.LightType != LightComponent::Type::Point)
            {
                // Negate the forward vector so it points in the direction of the light's +Z
                glm::vec3 forwardVector = -transformData.ForwardVec;
                m_Buffer->SetBytes(&forwardVector, sizeof(forwardVector), offset);
            }
            offset += sizeof(glm::vec4);

            // Update the rest of the light data after the transform
            m_Buffer->SetBytes(&lightComp, sizeof(lightComp), offset);

            // Add to the directional buffer if this light is directional
            if (lightComp.LightType == LightComponent::Type::Directional)
            {
                u32 dirOffset = directionalIndex++ * 4;
                m_DirectionalBuffer->SetBytes(&lightIndex, sizeof(lightIndex), dirOffset);
            }

            if (m_UseRayTracing && lightComp.LightType != LightComponent::Type::Directional)
            {
                // TODO: having to construct a transform component is mid
                m_Transforms.AddInPlace(TransformComponent{
                    transformData.Position,
                    transformData.Rotation,
                    glm::vec3(lightComp.Radius)
                }.GetTransformMatrix());
                instance.TransformMatrix = glm::value_ptr(m_Transforms.Back());
                instance.CustomIndex = lightIndex;
                m_Instances.AddInPlace(instance);
            }

            lightIndex++;
        }

        // Update the first element of the light buffer to contain the number of lights
        float lightCount = static_cast<float>(lightIndex - 1);
        m_Buffer->SetBytes(&lightCount, sizeof(float), 0);
        u32 directionalCount = directionalIndex - 1;
        m_DirectionalBuffer->SetBytes(&directionalCount, sizeof(u32), 0);

        // Build TLAS
        if (m_UseRayTracing)
        {
            auto encoder = m_CommandBuffer->EncodeComputeCommands();
            Flourish::AccelerationStructureSceneBuildInfo buildInfo;
            buildInfo.Instances = m_Instances.Data();
            buildInfo.InstanceCount = m_Instances.Count();
            encoder->RebuildAccelerationStructureScene(m_LightTLAS.get(), buildInfo);
            encoder->EndEncoding();
        }
    }
}
