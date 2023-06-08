#include "hepch.h"
#include "Material.h"

#include "Flourish/Api/DescriptorSet.h"
#include "Flourish/Api/DescriptorSetAllocator.h"
#include "Heart/Asset/AssetManager.h"
#include "Heart/Asset/TextureAsset.h"

namespace Heart
{
    void Material::Initialize()
    {
        Flourish::DescriptorSetAllocatorCreateInfo dsaCreateInfo;
        dsaCreateInfo.Compatability = Flourish::DescriptorSetPipelineCompatabilityFlags::Graphics;
        dsaCreateInfo.Layout = {
            { 0, Flourish::ShaderResourceType::Texture, Flourish::ShaderTypeFlags::Fragment, 1 },
            { 1, Flourish::ShaderResourceType::Texture, Flourish::ShaderTypeFlags::Fragment, 1 },
            { 2, Flourish::ShaderResourceType::Texture, Flourish::ShaderTypeFlags::Fragment, 1 },
            { 3, Flourish::ShaderResourceType::Texture, Flourish::ShaderTypeFlags::Fragment, 1 },
            { 4, Flourish::ShaderResourceType::Texture, Flourish::ShaderTypeFlags::Fragment, 1 }
        };

        s_DescriptorSetAllocator = Flourish::DescriptorSetAllocator::Create(dsaCreateInfo);
    }

    void Material::Shutdown()
    {
        s_DescriptorSetAllocator.reset();
    }

    bool BindTextureToIndex(const Ref<Flourish::Texture>& defaultTex, const Ref<Flourish::DescriptorSet>& set, UUID texId, u32 bindIndex)
    {
        auto texAsset = AssetManager::RetrieveAsset<TextureAsset>(texId);

        bool valid = texAsset && texAsset->IsValid();
        if (valid)
            set->BindTexture(bindIndex, texAsset->GetTexture());
        else
            set->BindTexture(bindIndex, defaultTex);

        return valid;
    }

    void Material::RecomputeDescriptorSet()
    {
        Flourish::DescriptorSetCreateInfo dsCreateInfo;
        dsCreateInfo.Writability = Flourish::DescriptorSetWritability::OnceStaticData;
        dsCreateInfo.StoreBindingReferences = true;

        auto set = s_DescriptorSetAllocator->Allocate(dsCreateInfo);

        const auto& defaultTex = AssetManager::RetrieveAsset<TextureAsset>("engine/DefaultTexture.png", true)->GetTexture();
        m_MaterialData.SetHasAlbedo(BindTextureToIndex(defaultTex, set, GetAlbedoTexture(), 0));
        m_MaterialData.SetHasMetallicRoughness(BindTextureToIndex(defaultTex, set, GetMetallicRoughnessTexture(), 1));
        m_MaterialData.SetHasNormal(BindTextureToIndex(defaultTex, set, GetNormalTexture(), 2));
        m_MaterialData.SetHasEmissive(BindTextureToIndex(defaultTex, set, GetEmissiveTexture(), 3));
        m_MaterialData.SetHasOcclusion(BindTextureToIndex(defaultTex, set, GetOcclusionTexture(), 4));

        set->FlushBindings();
        m_DescriptorSet = set;
    }
}
