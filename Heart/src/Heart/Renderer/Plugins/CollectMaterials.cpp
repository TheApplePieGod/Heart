#include "hepch.h"
#include "CollectMaterials.h"

#include "Flourish/Api/Context.h"
#include "Flourish/Api/Buffer.h"
#include "Flourish/Api/ComputeCommandEncoder.h"
#include "Flourish/Api/ResourceSetAllocator.h"
#include "Heart/Core/Timing.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/MeshAsset.h"
#include "Heart/Asset/MaterialAsset.h"
#include "Heart/Asset/TextureAsset.h"
#include "Heart/Renderer/SceneRenderer.h"

namespace Heart::RenderPlugins
{
    void CollectMaterials::InitializeInternal()
    {
        Flourish::BufferCreateInfo bufCreateInfo;
        bufCreateInfo.Usage = Flourish::BufferUsageType::Dynamic;
        bufCreateInfo.Type = Flourish::BufferType::Storage;
        bufCreateInfo.Stride = sizeof(MaterialInfo);
        bufCreateInfo.ElementCount = m_MaxMaterials;
        m_MaterialBuffer = Flourish::Buffer::Create(bufCreateInfo);

        Flourish::ResourceSetAllocatorCreateInfo setAllocCreateInfo;
        setAllocCreateInfo.Compatability = Flourish::ResourceSetPipelineCompatabilityFlags::All;
        setAllocCreateInfo.Layout = {
            {
                0,
                Flourish::ShaderResourceType::Texture,
                Flourish::ShaderTypeFlags::All,
                m_MaxTextures
            }
        };
        auto setAllocator = Flourish::ResourceSetAllocator::Create(setAllocCreateInfo);
        Flourish::ResourceSetCreateInfo setCreateInfo;
        setCreateInfo.Writability = Flourish::ResourceSetWritability::PerFrame;
        m_TexturesSet = setAllocator->Allocate(setCreateInfo);
    }

    int CollectMaterials::BindTexture(UUID texId, bool async)
    {
        auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(texId, true, async);
        bool valid = texAsset && texAsset->IsValid();
        if (valid)
        {
            m_TexturesSet->BindTexture(0, texAsset->GetTexture(), m_TextureIndex);
            return m_TextureIndex++;
        }

        return -1;
    }

    void CollectMaterials::AddMaterial(Material* material, bool async)
    {
        // TODO: probably should use UUIDs in the map, but that won't work
        // right now because default materials don't have ids
        u64 matId = (u64)material;
        if (m_MaterialMap.find(matId) != m_MaterialMap.end())
            return;
        m_MaterialMap.insert({ matId, m_MaterialIndex });

        MaterialInfo matInfo;
        matInfo.Data = material->GetMaterialData();
        matInfo.AlbedoIndex = BindTexture(material->GetAlbedoTexture(), async);
        matInfo.MetallicRoughnessIndex = BindTexture(material->GetMetallicRoughnessTexture(), async);
        matInfo.NormalIndex = BindTexture(material->GetNormalTexture(), async);
        matInfo.EmissiveIndex = BindTexture(material->GetEmissiveTexture(), async);
        matInfo.OcclusionIndex = BindTexture(material->GetOcclusionTexture(), async);
        m_MaterialBuffer->SetElements(&matInfo, 1, m_MaterialIndex++);
    }

    void CollectMaterials::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::CollectMaterials");

        m_MaterialMap.clear();
        m_MaterialIndex = 0;
        m_TextureIndex = 0;

        // Bind default material (don't bother with textures)
        {
            auto defMat = AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true);
            MaterialInfo matInfo = { defMat->GetMaterial().GetMaterialData() };
            m_MaterialBuffer->SetElements(&matInfo, 1, 0);
            m_MaterialMap.insert({ 0, 0 });
            m_MaterialIndex++;
        }

        // Add all materials from mesh components
        auto meshView = data.Scene->GetRegistry().view<MeshComponent>();
        for (entt::entity entity : meshView)
        {
            const auto& meshComp = meshView.get<MeshComponent>(entity);

            auto meshAsset = AssetManager::RetrieveAsset<MeshAsset>(
                meshComp.Mesh,
                true,
                data.Settings.AsyncAssetLoading
            );
            if (!meshAsset || !meshAsset->IsValid()) continue;

            for (u32 i = 0; i < meshAsset->GetSubmeshCount(); i++)
            {
                auto& meshData = meshAsset->GetSubmesh(i);

                Material* selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()];
                if (meshData.GetMaterialIndex() < meshComp.Materials.Count())
                {
                    auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(
                        meshComp.Materials[meshData.GetMaterialIndex()],
                        true,
                        data.Settings.AsyncAssetLoading
                    );
                    if (materialAsset && materialAsset->IsValid())
                        selectedMaterial = &materialAsset->GetMaterial();
                }

                AddMaterial(selectedMaterial, data.Settings.AsyncAssetLoading);
            }
        }

        // Add all materials from text components
        auto textView = data.Scene->GetRegistry().view<TextComponent>();
        for (entt::entity entity : textView)
        {
            const auto& textComp = textView.get<TextComponent>(entity);

            Material* selectedMaterial = nullptr;
            auto materialAsset = AssetManager::RetrieveAsset<MaterialAsset>(
                textComp.Material,
                true,
                data.Settings.AsyncAssetLoading
            );
            if (materialAsset && materialAsset->IsValid())
                selectedMaterial = &materialAsset->GetMaterial();

            AddMaterial(selectedMaterial, data.Settings.AsyncAssetLoading);
        }

        m_TexturesSet->FlushBindings();
    }
}
