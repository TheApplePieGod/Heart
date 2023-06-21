#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "Flourish/Api/RayTracing/AccelerationStructure.h"

namespace Flourish
{
    class Buffer;
    class ResourceSet;
};

namespace Heart::RenderPlugins
{
    class TLAS : public RenderPlugin
    {
    public:
        struct ObjectData
        {
            void* VertexBufferAddress;
            void* IndexBufferAddress;
        };

    public:
        TLAS(SceneRenderer* renderer, HStringView8 name)
            : RenderPlugin(renderer, name)
        { Initialize(); }

        inline const Flourish::AccelerationStructure* GetAccelStructure() const { return m_AccelStructure.get(); }
        inline const Flourish::Buffer* GetObjectBuffer() const { return m_ObjectBuffer.get(); }
        inline const Flourish::Buffer* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }
        inline const Flourish::ResourceSet* GetTexturesSet() const { return m_TexturesSet.get(); }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();

    private:
        u32 m_MaxObjects = 5000;
        HVector<Flourish::AccelerationStructureInstance> m_Instances;
        Ref<Flourish::AccelerationStructure> m_AccelStructure;
        Ref<Flourish::ResourceSet> m_TexturesSet;
        Ref<Flourish::Buffer> m_ObjectBuffer;
        Ref<Flourish::Buffer> m_MaterialBuffer;
    };
}
