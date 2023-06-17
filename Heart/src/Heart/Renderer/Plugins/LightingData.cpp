#include "hepch.h"
#include "LightingData.h"

#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart::RenderPlugins
{
    void LightingData::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();

        u32 lightIndex = 1;
        for (const auto& lightComp : data.Scene->GetLightComponents())
        {
            if (lightIndex >= m_MaxLights)
                break;

            const auto& entityData = data.Scene->GetEntityData()[lightComp.EntityIndex];

            u32 offset = lightIndex * m_Buffer->GetStride();

            if (lightComp.Data.LightType == LightComponent::Type::Disabled) continue;

            // Update the translation part of the light struct
            m_Buffer->SetBytes(&entityData.Translation, sizeof(glm::vec3), offset);
            offset += sizeof(glm::vec4);

            // Update the light direction if the light is not a point light
            if (lightComp.Data.LightType != LightComponent::Type::Point)
            {
                // Negate the forward vector so it points in the direction of the light's +Z
                glm::vec3 forwardVector = -entityData.ForwardVec;
                m_Buffer->SetBytes(&forwardVector, sizeof(forwardVector), offset);
            }
            offset += sizeof(glm::vec4);

            // Update the rest of the light data after the transform
            m_Buffer->SetBytes(&lightComp.Data, sizeof(lightComp.Data), offset);

            lightIndex++;
        }

        // Update the first element of the light buffer to contain the number of lights
        float lightCount = static_cast<float>(lightIndex - 1);
        m_Buffer->SetBytes(&lightCount, sizeof(float), 0);
    }
    
    void LightingData::Initialize()
    {
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(LightData);
        bufCreateInfo.ElementCount = m_MaxLights;
        m_Buffer = Flourish::Buffer::Create(bufCreateInfo);
    }
}
