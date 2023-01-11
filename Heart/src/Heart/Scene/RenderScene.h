#pragma once

#include "Heart/Container/HVector.hpp"
#include "Heart/Scene/Components.h"
#include "glm/mat4x4.hpp"

namespace Heart
{
    class Scene;
    class RenderScene
    {
    public:
        struct EntityData
        {
            EntityData(u32 id, const glm::mat4& transform, glm::vec3 tran, glm::vec3 rot, glm::vec3 scale, glm::vec3 forward)
                : Id(id), Transform(transform), Translation(tran), Rotation(rot), Scale(scale), ForwardVec(forward)
            {}

            u32 Id;
            glm::mat4 Transform;
            glm::vec3 Translation;
            glm::vec3 Rotation;
            glm::vec3 Scale;
            glm::vec3 ForwardVec;
        };
        
        template <typename Comp>
        struct ComponentData
        {
            ComponentData(const Comp& data, u32 entityIndex)
                : Data(data), EntityIndex(entityIndex)
            {}

            Comp Data;
            u32 EntityIndex;
        };

    public:
        RenderScene() = default;
        
        void CopyFromScene(Scene* scene);
        
        inline bool IsInitialized() const { return m_Initialized; }
        inline const auto& GetEntityData() const { return m_EntityData; }
        inline auto& GetMeshComponents() const { return m_MeshComponents; }
        inline auto& GetTextComponents() const { return m_TextComponents; }
        inline auto& GetLightComponents() const { return m_LightComponents; }
        
        inline static constexpr u32 InvalidIndex = std::numeric_limits<u32>::max();

    private:
        HVector<EntityData> m_EntityData;
        HVector<ComponentData<MeshComponent>> m_MeshComponents;
        HVector<ComponentData<TextComponent>> m_TextComponents;
        HVector<ComponentData<LightComponent>> m_LightComponents;
        
        bool m_Initialized = false;
    };
}