#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"
#include "glm/vec4.hpp"

namespace Flourish
{
    class Buffer;
};

namespace Heart::RenderPlugins
{
    struct TLASCreateInfo
    {
        HString8 CollectMaterialsPluginName;
    };

    class TLAS : public RenderPlugin
    {
    public:
        struct ObjectData
        {
            void* VertexBufferAddress;
            void* IndexBufferAddress;
            glm::vec4 Data; // R: MaterialId
        };

    public:
        TLAS(SceneRenderer* renderer, HStringView8 name, const TLASCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline const Flourish::AccelerationStructure* GetAccelStructure() const { return m_AccelStructure.get(); }
        inline const Flourish::Buffer* GetObjectBuffer() const { return m_ObjectBuffer.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        TLASCreateInfo m_Info;

        u32 m_MaxObjects = 10000;
        HVector<Flourish::AccelerationStructureInstance> m_Instances;
        Ref<Flourish::AccelerationStructure> m_AccelStructure;
        Ref<Flourish::Buffer> m_ObjectBuffer;
    };
}
