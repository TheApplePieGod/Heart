#pragma once

#include "Heart/Renderer/RenderPlugin.h"
#include "Heart/Renderer/Material.h"
#include "Flourish/Api/ResourceSet.h"

namespace Flourish
{
    class Buffer;
};

namespace Heart::RenderPlugins
{
    struct CollectMaterialsCreateInfo
    {
        
    };

    class CollectMaterials : public RenderPlugin
    {
    public:
        struct alignas(16) MaterialInfo
        {
            // TODO: could trim the hasTextures parameter out
            MaterialData Data;
            int AlbedoIndex = -1;
            int MetallicRoughnessIndex = -1;
            int NormalIndex = -1;
            int EmissiveIndex = -1;
            int OcclusionIndex = -1;
        };

    public:
        CollectMaterials(SceneRenderer* renderer, HStringView8 name, const CollectMaterialsCreateInfo& createInfo)
            : RenderPlugin(renderer, name), m_Info(createInfo)
        { Initialize(); }

        inline const Flourish::Buffer* GetMaterialBuffer() const { return m_MaterialBuffer.get(); }
        inline const Flourish::ResourceSet* GetTexturesSet() const { return m_TexturesSet.get(); }
        inline const auto& GetMaterialMap() const { return m_MaterialMap; }

    protected:
        void RenderInternal(const SceneRenderData& data) override;
        void ResizeInternal() override {};

    private:
        void Initialize();
        int BindTexture(UUID texId, bool async);
        void AddMaterial(Material* material, bool async);

    private:
        CollectMaterialsCreateInfo m_Info;

        u32 m_TextureIndex = 0;
        u32 m_MaterialIndex = 0;

        Ref<Flourish::ResourceSet> m_TexturesSet;
        Ref<Flourish::Buffer> m_MaterialBuffer;
        std::unordered_map<u64, u32> m_MaterialMap;

        u32 m_MaxMaterials = 5000;
        u32 m_MaxTextures = 1024;
    };
}
