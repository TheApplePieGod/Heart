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
    void CollectMaterials::Initialize()
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

    void CollectMaterials::RenderInternal(const SceneRenderData& data)
    {
        HE_PROFILE_FUNCTION();
        auto timer = AggregateTimer("RenderPlugins::CollectMaterials");

        m_MaterialMap.clear();

        int arrayIndex = 0;
        auto& texturesSet = m_TexturesSet;
        auto bindTex = [&arrayIndex, &texturesSet, &data](UUID texId, int& outIndex)
        {
            auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(texId, true, data.Settings.AsyncAssetLoading);
            bool valid = texAsset && texAsset->IsValid();
            if (valid)
            {
                texturesSet->BindTexture(0, texAsset->GetTexture(), arrayIndex);
                outIndex = arrayIndex++;
            }
        };

        // Bind default material (don't bother with textures)
        {
            auto defMat = AssetManager::RetrieveAsset<MaterialAsset>("engine/DefaultMaterial.hemat", true);
            MaterialInfo matInfo = { defMat->GetMaterial().GetMaterialData() };
            m_MaterialBuffer->SetElements(&matInfo, 1, 0);
            m_MaterialMap.insert({ 0, 0 });
        }

        u32 materialIndex = 1;
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

                auto selectedMaterial = &meshAsset->GetDefaultMaterials()[meshData.GetMaterialIndex()];
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

                // TODO: probably should use UUIDs in the map, but that won't work
                // right now because default materials don't have ids
                u64 matId = (u64)selectedMaterial;
                if (m_MaterialMap.find(matId) != m_MaterialMap.end())
                    continue;
                m_MaterialMap.insert({ matId, materialIndex });

                MaterialInfo matInfo;
                matInfo.Data = selectedMaterial->GetMaterialData();
                bindTex(selectedMaterial->GetAlbedoTexture(), matInfo.AlbedoIndex);
                bindTex(selectedMaterial->GetMetallicRoughnessTexture(), matInfo.MetallicRoughnessIndex);
                bindTex(selectedMaterial->GetNormalTexture(), matInfo.NormalIndex);
                bindTex(selectedMaterial->GetEmissiveTexture(), matInfo.EmissiveIndex);
                bindTex(selectedMaterial->GetOcclusionTexture(), matInfo.OcclusionIndex);
                m_MaterialBuffer->SetElements(&matInfo, 1, materialIndex++);
            }
        }

        texturesSet->FlushBindings();
    }
}
