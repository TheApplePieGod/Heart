#include "htpch.h"
#include "Asset.h"

#include "Heart/Asset/TextureAsset.h"
#include "Heart/Asset/ShaderAsset.h"
#include "Heart/Asset/MeshAsset.h"

namespace Heart
{
    Asset::Asset(const std::string& path)
        : m_Path(path)
    {
        auto entry = std::filesystem::path(path);
        m_Filename = entry.filename().generic_u8string();
        m_Extension = entry.extension().generic_u8string();

        // convert the extension to lowercase
        std::transform(m_Extension.begin(), m_Extension.end(), m_Extension.begin(), [](unsigned char c) { return std::tolower(c); });
    }

    void Asset::Reload()
    {
        if (m_Loaded)
            Unload();
        Load();
    }

    Ref<Asset> Asset::Create(Type type, const std::string& path)
    {
        switch (type)
        {
            default:
            { HE_ENGINE_ASSERT(false, "Cannot create asset: selected type is not supported"); return nullptr; }
            case Asset::Type::Texture:
            { return CreateRef<TextureAsset>(path); }
            case Asset::Type::Shader:
            { return CreateRef<ShaderAsset>(path); }
            case Asset::Type::Mesh:
            { return CreateRef<MeshAsset>(path); }
        }
    }
}